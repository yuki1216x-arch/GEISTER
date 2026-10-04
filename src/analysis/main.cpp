// Analysis program for forced-win, can-lose, and forced-loss labels
// Database with 2 bits per entry
#include <deque>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <thread>
#include <map>
#include <stdint.h>
#include <climits>
#include <exception>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include "../common/table.hpp"
#include "../common/posi_geister.hpp"
#include "../common/zdd_geister.hpp"

using std::ref;
using std::cref;

using namespace std;

// int NUM_B, NUM_R, NUM_EB, NUM_ER;

constexpr int nworker = 7;  // number of parallel threads
constexpr int deq_input_size = 2048;    
constexpr int deq_output_size = 256;
constexpr int max_legal_num = 32;
constexpr int max_belief_state = 70;

condition_variable cv_boss;     //condition_variable are also used for thread synchronization
condition_variable cv_worker;   
mutex mtx;  
// POSIX threading
// name of the POSIX interface
// use a POSIX thread mutex here

bool flag_worker_quit; // flag indicating whether all configurations have been analyzed once
int flag = 0;
unsigned long long int search_id = 0ULL;

// class representing a task
class Work {
private:
  unsigned long long int m_id;   // assigned ID
  unsigned int m_value;   // original value
  int m_nchild;   // number of legal moves for m_id
  int m_num_of_un;   // number of third-party configurations predicted for m_id
  // After applying the move and configuration
  int m_captured_piece_type[max_legal_num][max_belief_state]; // color of the captured piece
  long long int m_array_id[max_legal_num][max_belief_state]; // ID of the non-terminal configuration
  int m_captured_piece_type_opp[max_belief_state][max_legal_num];
  long long int m_array_id_opp[max_belief_state][max_legal_num];
  long long int m_array_id_opp_e[max_belief_state];

public:
  Work() noexcept {}
  Work(unsigned long long int id) noexcept : m_id(id) {}
  void set_id(unsigned long long int id, unsigned int value) noexcept {
    m_id = id;
    m_value = value;
  }
  void set(int nchild, int num_of_un, int captured_piece_type[max_legal_num][max_belief_state], long long int array_id[max_legal_num][max_belief_state]) noexcept {
    assert(nchild > 0 && nchild <= max_legal_num);
    assert(num_of_un > 0 && num_of_un <= max_belief_state);
    m_nchild = nchild;
    m_num_of_un = num_of_un;
    for(int i = 0; i < m_nchild; i++) {
      for(int j = 0; j < m_num_of_un; j++) {
	m_captured_piece_type[i][j] = captured_piece_type[i][j];
	m_array_id[i][j] = array_id[i][j];
      }
    }
  }
  void set_opp(int num_of_un, long long int array_id_opp_e[max_belief_state]) noexcept {
    assert(num_of_un > 0 && num_of_un <= max_belief_state);
    m_num_of_un = num_of_un;
    for(int i = 0; i < m_num_of_un; i++) {
      m_array_id_opp_e[i] = array_id_opp_e[i];
    }
  }
  void set_opp(int nchild, int num_of_un, int captured_piece_type_opp[max_belief_state][max_legal_num], long long int array_id_opp[max_belief_state][max_legal_num]) noexcept {
    assert(nchild > 0 && nchild <= max_legal_num);
    assert(num_of_un > 0 && num_of_un <= max_belief_state);
    m_nchild = nchild;
    m_num_of_un = num_of_un;
    for(int i = 0; i < m_num_of_un; i++) {
      for(int j = 0; j < m_nchild; j++) {
	m_captured_piece_type_opp[i][j] = captured_piece_type_opp[i][j];
	m_array_id_opp[i][j] = array_id_opp[i][j];
      }
    }
  }
  unsigned long long int get_id() const noexcept { return m_id; }
  unsigned int get_value() const noexcept { return m_value; }
  void get(int& nchild, int& num_of_un, int captured_piece_type[max_legal_num][max_belief_state], long long int array_id[max_legal_num][max_belief_state]) const noexcept {
    assert(m_nchild > 0 && m_nchild <= max_legal_num);
    assert(m_num_of_un > 0 && m_num_of_un <= max_belief_state);
    nchild = m_nchild;
    num_of_un = m_num_of_un;
    for(int i = 0; i < m_nchild; i++) {
      for(int j = 0; j < m_num_of_un; j++) {
	captured_piece_type[i][j] = m_captured_piece_type[i][j];
	array_id[i][j] = m_array_id[i][j];
      }
    }
  }
  void get_opp(int& num_of_un, long long int array_id_opp_e[max_belief_state]) const noexcept {
    assert(m_num_of_un > 0 && m_num_of_un <= max_belief_state);
    num_of_un = m_num_of_un;
    for(int i = 0; i < m_num_of_un; i++) {
      array_id_opp_e[i] = m_array_id_opp_e[i];
    }
  }
  void get_opp(int& nchild, int& num_of_un, int captured_piece_type_opp[max_belief_state][max_legal_num], long long int array_id_opp[max_belief_state][max_legal_num]) const noexcept {
    assert(m_nchild > 0 && m_nchild <= max_legal_num);
    assert(m_num_of_un > 0 && m_num_of_un <= max_belief_state);
    nchild = m_nchild;
    num_of_un = m_num_of_un;
    for(int i = 0; i < m_num_of_un; i++) {
      for(int j = 0; j < m_nchild; j++) {
	captured_piece_type_opp[i][j] = m_captured_piece_type_opp[i][j];
	array_id_opp[i][j] = m_array_id_opp[i][j];
      }
    }
  }
};

constexpr unsigned long long int placement_count[5][5][9] {
  { {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL} }
  , { {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 706860ULL, 7539840ULL, 58433760ULL, 350602560ULL, 1694579040ULL, 6778316160ULL, 22876817040ULL}
    , {0ULL, 0ULL, 11309760ULL, 116867520ULL, 876506400ULL, 5083737120ULL, 23724106560ULL, 91507268160ULL, 297398621520ULL}
    , {0ULL, 0ULL, 116867520ULL, 1168675200ULL, 8472895200ULL, 47448213120ULL, 213516959040ULL, 793062990720ULL, 2478321846000ULL}
    , {0ULL, 0ULL, 876506400ULL, 8472895200ULL, 59310266400ULL, 320275438560ULL, 1387860233760ULL, 4956643692000ULL, 14869931076000ULL} }
  , { {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 11309760ULL, 116867520ULL, 876506400ULL, 5083737120ULL, 23724106560ULL, 91507268160ULL, 297398621520ULL}
    , {0ULL, 0ULL, 175301280ULL, 1753012800ULL, 12709342800ULL, 71172319680ULL, 320275438560ULL, 1189594486080ULL, 3717482769000ULL}
    , {0ULL, 0ULL, 1753012800ULL, 16945790400ULL, 118620532800ULL, 640550877120ULL, 2775720467520ULL, 9913287384000ULL, 29739862152000ULL}
    , {0ULL, 0ULL, 12709342800ULL, 118620532800ULL, 800688596400ULL, 4163580701280ULL, 17348252922000ULL, 59479724304000ULL, 171004207374000ULL} }
  , { {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 116867520ULL, 1168675200ULL, 8472895200ULL, 47448213120ULL, 213516959040ULL, 793062990720ULL, 2478321846000ULL}
    , {0ULL, 0ULL, 1753012800ULL, 16945790400ULL, 118620532800ULL, 640550877120ULL, 2775720467520ULL, 9913287384000ULL, 29739862152000ULL}
    , {0ULL, 0ULL, 16945790400ULL, 158160710400ULL, 1067584795200ULL, 5551440935040ULL, 23131003896000ULL, 79306299072000ULL, 228005609832000ULL}
    , {0ULL, 0ULL, 118620532800ULL, 1067584795200ULL, 6939301168800ULL, 34696505844000ULL, 138786023376000ULL, 456011219664000ULL, 1254030854076000ULL} }
  , { {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 876506400ULL, 8472895200ULL, 59310266400ULL, 320275438560ULL, 1387860233760ULL, 4956643692000ULL, 14869931076000ULL}
    , {0ULL, 0ULL, 12709342800ULL, 118620532800ULL, 800688596400ULL, 4163580701280ULL, 17348252922000ULL, 59479724304000ULL, 171004207374000ULL}
    , {0ULL, 0ULL, 118620532800ULL, 1067584795200ULL, 6939301168800ULL, 34696505844000ULL, 138786023376000ULL, 456011219664000ULL, 1254030854076000ULL}
    , {0ULL, 0ULL, 800688596400ULL, 6939301168800ULL, 43370632305000ULL, 208179035064000ULL, 798019634412000ULL, 2508061708152000ULL, 6583661983899000ULL} }
};

const string base[2] = {"self_table", "enemy_table"};
	  
unsigned long long int nwin = 0, nlose = 0, ncan_lose = 0; // number of newly assigned labels of each type (for verigication)

unsigned long long int count_changes = 0ULL;   // number of updates (stop the search when it reaches 0)

// queue for exchanging tasks between the boss and workers
// the boss stores tasks here, and workers retrieve them to find legal moves
deque<Work *> deq_input;
// workers store the computed IDs here, and the boss writes value to the table
deque<Work *> deq_output;

// Boss function
static void boss(int iter, int num_b, int num_r, int num_eb, int num_er,
		 Table& parent_table, const char* write_filename,
		 const Table& child_table, const Table& child_table_cap_b, const Table& child_table_cap_r,
		 const Table& child_table_opp) noexcept {
  std::cout << "boss" << endl;

  nwin = 0, nlose = 0, ncan_lose = 0;
  count_changes = 0ULL;
  
  unsigned long long int max_placement = placement_count[num_b][num_r][num_eb + num_er];
  unsigned long long int count_input = 0ULL;  // number of configurations checked to determine whether they should be analyed in this iteration
  unsigned long long int count_output = 0ULL; // number of configurations whose processing has been completed
  
  int nstack_work_idle = deq_input_size + deq_output_size + nworker;  // number of tasks that can currently be processed
  Work* stack_work_idle[nstack_work_idle];    // array for storing tasks
  for(int i = 0; i < nstack_work_idle; i++) stack_work_idle[i] = new Work;  // initialize the array with empty tasks
  
  while(true) {  // repeat until there are no tasks left
    unique_lock<mutex> lck(mtx);  // lock the mutex; mtx is automatically unlocked when the unique_lock instance lck is destroyed
    cv_boss.wait(lck, [&](){  // wait (unique_lock instance, lambda expression capturing by reference)
			return (((deq_input.size() < deq_input_size) && (deq_output.size() < deq_output_size) && (count_input < max_placement))
				|| (0 < deq_output.size())); });
    // wait until deq_input and deq_output have enough free space
    // wait blocks while the condition is false and returns when the condition becomes true
    // Condition : the return value of the function passed as the second argument
    // the condition may be checked at any time
    // while wait is blocking, the lock passed as the first argument (lck) is released
    // releasing the lock and entering the waiting state are performed atomically

    // when wait returns, the lock has been reacquired
    // after waking up, the predicate is evaluated again; if false, the lock is released and the thread waits again
    // if true, wait returns
    
    // if deq_input and deq_output are small enough, find an ID that needs to be analyzed and create a task for it (Step 1)
    if ((deq_input.size() < deq_input_size) && (deq_output.size() < deq_output_size) && count_input < max_placement) {
      // retrieve the value from the retrograde analysis table
      // continue the analysis if the label is unknown or can-lose
      unsigned int before_value;
      while(count_input < max_placement && (before_value = parent_table.get(count_input)) != v_unknown && before_value != v_can_lose) {
	// keep incrementing the ID until an unknown or can-lose label is found
	count_input++;
	count_output++;
      }

      if(count_input >= max_placement) {
	lck.unlock();
	continue;
      }

      // if(count_input % 1000000000ULL == 0ULL) std::cout << "count_input: " << count_input << ", nwin: " << nwin << ", nexist_lose: " << ncan_lose << ", nunknown: " << nunknown << endl;
                
      assert(nstack_work_idle >= 1);
      Work *pw = stack_work_idle[ --nstack_work_idle ];   // retrieve a task to assign from stack_work_idle
      pw->set_id(count_input, before_value);    // set the configuration ID and the value assigned by the previous analysis for the worker
      deq_input.push_front(pw);   // add a task
      lck.unlock();
      count_input++;
      
      if(count_input % 1000000000ULL == 0ULL) std::cout << "count_input: " << count_input << ", nwin: " << nwin << ", ncan_lose: " << ncan_lose << ", nlose: " << nlose << endl;
      
      cv_worker.notify_one();   // wake up one worker thread
    } else {    // if tasks have accumulated (step 3)
      if (0 < deq_output.size()) {
	assert(0 < deq_output.size());
	deque<Work *> deq_tmp; // use swap
	swap(deq_tmp, deq_output);
	
	// empty deq_output
	deq_output.clear();
	lck.unlock();   // swap with deq_tmp, which is not accessed by other threads, to release the lock sooner
	count_output += deq_tmp.size();
	for (unsigned int workid = 0; workid < deq_tmp.size(); workid++) {
	  unsigned long long int id = deq_tmp[workid]->get_id();  // retrieve the assigned ID from the task
	  unsigned int value = deq_tmp[workid]->get_value();
	  
	  if(value == v_unknown) {   // if the previous value is unknown
	    int nchild, num_of_un;
	    int captured_piece_type[max_legal_num][max_belief_state];
	    long long int array_id[max_legal_num][max_belief_state];
	    // retrieve all information needed to determine forced-win and can-lose labels
	    deq_tmp[workid]->get(nchild, num_of_un, captured_piece_type, array_id);
	    bool already_decided = false;
	    int num_of_action = nchild;
	    
	    assert(nchild > 0);   // Invalid: there should be at least one child
	    int win_plan_num = 0;  // number of forced-win strategies
	    int can_lose_plan_num = 0;  // number of exist-loss strategies
	    for(int j = 0; j < num_of_action; j++) {
	      int kati = 0;
	      int make = 0;
	      int makeari = 0;
	      int humei = 0;
	      for(int k = 0; k < num_of_un; k++) {
		long long int num_of_haiti = array_id[j][k];
		if(num_of_haiti >= 0) {   // not terminal; retrieve the value from the database
		  int child_val;
		  int which_table = captured_piece_type[j][k];
		  // no piece captured
		  if(which_table == 0) child_val = child_table.get((unsigned long long int)num_of_haiti);
		  // blue piece captured
		  else if(which_table == 1) child_val = child_table_cap_b.get((unsigned long long int)num_of_haiti);
		  // red piece captured
		  else {
		    assert(which_table == 2);
		    child_val = child_table_cap_r.get((unsigned long long int)num_of_haiti);
		  }
		  
		  if(child_val == v_win) kati++;
		  else if(child_val == v_lose) make++;
		  else if(child_val == v_can_lose) makeari++;
		  else {
		    assert(child_val == v_unknown);
		    humei++;
		  }
		} else {
		  if(num_of_haiti == -1) {
		    // player1 wins
		    kati++;
		  } else if(num_of_haiti == -2) {
		    // player2 loses
		    make++;
		  } else {
		    // Do not count this move because it is illegal
		  }
		}
	      }
	      
	      if(kati == num_of_un) { // forced win
		if(iter % 2 == 1) {
		  if(!already_decided) {
		    parent_table.set(id, v_win);
		    nwin++, count_changes++;
		    already_decided = true;
		    value = v_win;
		  }
		} else {
		  win_plan_num++;
		}
	      } else if(make + makeari >= 1) { // can lose
		if(iter % 2 == 1) {
		  can_lose_plan_num++;
		} else {
		  if(!already_decided) {
		    parent_table.set(id, v_can_lose);
		    value = v_can_lose;
		    ncan_lose++;
		    count_changes++;
		    already_decided = true;
		  }
		}
	      }
	    }
	  
	    // after checking all legal moves
	    if(!already_decided) {
	      if(iter % 2 == 1) {
		if(can_lose_plan_num == num_of_action) {  // can lose
		  parent_table.set(id, v_can_lose);
		  value = v_can_lose;
		  ncan_lose++;
		  count_changes++;
		}
	      } else {
		if(win_plan_num == num_of_action) { // forced win
		  parent_table.set(id, v_win);
		  nwin++, count_changes++;
		  value = v_win;
		}
	      }
	    }
	  }

	  if(value == v_unknown || value == v_can_lose) {   // if the previous value is unknown or can-lose
	    if(iter % 2 == 1) {
	      int nchild, num_of_un;
	      int captured_piece_type_opp[max_belief_state][max_legal_num];
	      long long int array_id_opp[max_belief_state][max_legal_num];
	      // retrieve all information needed to determine forced-loss labels
	      deq_tmp[workid]->get_opp(nchild, num_of_un, captured_piece_type_opp, array_id_opp);
	      int num_of_action = nchild;
	      bool already_decided = false;
	      int lose_plan_num = 0;  // number of loss strategies
	      assert(nchild > 0);   // Invalid: there should be at least one child
	      for(int j = 0; j < num_of_un; j++) {
		int kati = 0;
		int make = 0;
		int makeari = 0;
		int humei = 0;
		for(int k = 0; k < num_of_action; k++) {
		  long long int num_of_haiti = array_id_opp[j][k];
		
		  if(num_of_haiti >= 0) {   // not terminal; retrieve the value from the database
		    int child_val;
		    int which_table = captured_piece_type_opp[j][k];
		    // no piece captured
		    if(which_table == 0) child_val = child_table.get((unsigned long long int)num_of_haiti);
		    // blue piece captured
		    else if(which_table == 1) child_val = child_table_cap_b.get((unsigned long long int)num_of_haiti);
		    // red piece captured
		    else {
		      assert(which_table == 2);
		      child_val = child_table_cap_r.get((unsigned long long int)num_of_haiti);
		    }
		    
		    if(child_val == v_win) kati++;
		    else if(child_val == v_lose) make++;
		    else if(child_val == v_can_lose) makeari++;
		    else {
		      assert(child_val == v_unknown);
		      humei++;
		    }
		  } else {
		    if(num_of_haiti == -1) {
		      // player1 wins
		      kati++;
		    } else if(num_of_haiti == -2) {
		      // player2 loses
		      make++;
		    } else {
		      // Do not count this move because it is illegal
		    }
		  }
		}
	      
		if(make == num_of_action) { // forced loss
		  lose_plan_num++;
		}
	      }
	  
	      // after checking all legal moves
	      if(!already_decided) {
		if(lose_plan_num == num_of_un) { // forced loss
		  parent_table.set(id, v_lose);
		  nlose++, count_changes++;
		  value = v_lose;
		}
	      }
	    } else {
	      assert(iter % 2 == 0);
	      int num_of_un;
	      long long int array_id_opp_e[max_belief_state];
	      deq_tmp[workid]->get_opp(num_of_un, array_id_opp_e);
	      int kati = 0;
	      int make = 0;
	      int makeari = 0;
	      int humei = 0;
	      for(int j = 0; j < num_of_un; j++) {
		long long int num_of_haiti = array_id_opp_e[j];
		assert(num_of_haiti >= 0);
		int child_val = child_table_opp.get((unsigned long long int)num_of_haiti);
		    
		if(child_val == v_win) kati++;
		else if(child_val == v_lose) make++;
		else if(child_val == v_can_lose) makeari++;
		else {
		  assert(child_val == v_unknown);
		  humei++;
		}
	      }

	      if(kati == num_of_un) {
		parent_table.set(id, v_lose);
		nlose++, count_changes++;
		value = v_lose;
	      }
	    }
	  }
	}
	  
	// return the paper to the shelf
	for(size_t i = 0; i < deq_tmp.size(); i++){
	  assert(nstack_work_idle < 4096);
	  stack_work_idle[ nstack_work_idle++ ] = deq_tmp[i];
	}
      }
      
      // stop if the number of processed configurations exceeds the total number of configuration
      if (count_output >= max_placement) break;
    }
  } // when this scope ends, lck is destroyed and mtx is unlocked
  
  cout << "before_outtable" << endl;
  {
    OutTable out_table(iter, write_filename, max_placement, 2);
    for (unsigned long long int i = 0; i < max_placement; i++) {
      out_table.write(parent_table.get(i));   // retrieve the value of id(i) and copy it directly
    }
    out_table.flush();
    out_table.outinfo();
  }
  
  {
    unique_lock<mutex> lck(mtx); // lock
    flag_worker_quit = true; // flag indicating that the work is finished
  }
  cv_worker.notify_all();
}
  
static void worker(int iter, int num_b, int num_r, int num_eb, int num_er,
		   const ZDD& zdd_parent, const ZDD& zdd_child_cap_b, const ZDD& zdd_child_cap_r,
		   const ZDD& zdd_parent_opp) noexcept {
  Work *w;
  while (true) {   // repeat until all tasks are completed
    unique_lock<mutex> lck(mtx);   // lock
    // unlock the mutex
    cv_worker.wait(lck, [&](){ if (0 < deq_input.size()) return true;
	return flag_worker_quit; });   // wait until a task is available or all tasks have been completed
    // lock
    if (flag_worker_quit) break;   // exit if all tasks have been completed
    assert(0 < deq_input.size());   // at least one task is available
    w = deq_input.back();   // retrieve a task from deq_input (copy)
    deq_input.pop_back();   // remove the retrieved task
    lck.unlock();   // unlock the mutex used to prevent concurrent access to deq_input
    
    // set all child IDs or direct outcomes for the assigned configuration ID
    unsigned long long int id = w->get_id();
    unsigned int value = w->get_value();
    if(value == v_unknown) {
      Posi p;
      p.make_posi(id, zdd_parent, num_b, num_r, num_eb, num_er);   // create the position for a given ID
      Action actions[max_legal_num];
      unsigned char board_belief[70][36];
      int nchild = p.compute_actions(actions, iter);   // enumerate legal moves for the position
      int num_of_un = p.getunknowninfo(board_belief);   // enumerate all possible third-party configurations for the position
      assert(nchild > 0 && nchild < max_legal_num);
      assert(num_of_un > 0 && num_of_un <= max_belief_state);
      
      int captured_piece_type[max_legal_num][max_belief_state] = {};
      long long int array_id[max_legal_num][max_belief_state];
      
      for(int i = 0; i < nchild; i++) {
	for(int j = 0; j < num_of_un; j++) {
	  p.make_posi_n(board_belief, j);   // create a configuration from a third-party perspective
	  int board_check = p.make_action(actions[i]);   // apply the legal move
	  if(board_check >= 0) {   // the game is not decided directly
	    p.make_posi_myself();
	    if(board_check == 0) {
	      array_id[i][j] = p.getzddnum(zdd_parent);
	    } else if(board_check == 1) {
	      array_id[i][j] = p.getzddnum(zdd_child_cap_b);
	      captured_piece_type[i][j] = 1;
	    } else {
	      assert(board_check == 2);
	      array_id[i][j] = p.getzddnum(zdd_child_cap_r);
	      captured_piece_type[i][j] = 2;
	    }
	  } else {
	    array_id[i][j] = board_check;
	  }
	  p.undo_action();
	}
      }
      // set the child ID, captured piece, and other information
      w->set(nchild, num_of_un, captured_piece_type, array_id);
    }

    if(value == v_unknown || value == v_can_lose) {
      Posi p;
      p.make_posi(id, zdd_parent, num_b, num_r, num_eb, num_er);   // create the position for a given ID
      Action actions[max_legal_num];
      unsigned char board_belief[70][36];
      int nchild = p.compute_actions(actions, iter);   // enumerate legal moves for the position
      int num_of_un = p.getunknowninfo(board_belief);   // enumerate all possible third-party configurations for the position
      assert(nchild > 0 && nchild < max_legal_num);
      assert(num_of_un > 0 && num_of_un <= max_belief_state);

      if(iter % 2 == 1) {
	int captured_piece_type_opp[max_belief_state][max_legal_num] = {};
	long long int array_id_opp[max_belief_state][max_legal_num];
      
	for(int i = 0; i < num_of_un; i++) {
	  for(int j = 0; j < nchild; j++) {
	    p.make_posi_n(board_belief, i);   // create a configuration from a third-party perspective
	    int board_check = p.make_action(actions[j]);   // apply the legal move
	    if(board_check >= 0) {   // the game is not decided directly
	      p.make_posi_myself();
	      if(board_check == 0) {
		array_id_opp[i][j] = p.getzddnum(zdd_parent);
	      } else if(board_check == 1) {
		array_id_opp[i][j] = p.getzddnum(zdd_child_cap_b);
		captured_piece_type_opp[i][j] = 1;
	      } else {
		assert(board_check == 2);
		array_id_opp[i][j] = p.getzddnum(zdd_child_cap_r);
		captured_piece_type_opp[i][j] = 2;
	      }
	    } else {
	      array_id_opp[i][j] = board_check;
	    }
	    p.undo_action();
	  }
	}
	w->set_opp(nchild, num_of_un, captured_piece_type_opp, array_id_opp);
      } else {
	assert(iter % 2 == 0);
	long long int array_id_opp_e[max_belief_state];

	for(int i = 0; i < num_of_un; i++) {
	  p.make_posi_n(board_belief, i);
	  p.make_posi_opponent();
	  array_id_opp_e[i] = p.getzddnum(zdd_parent_opp);
	}
	w->set_opp(num_of_un, array_id_opp_e);
      }
    }
    
    lck.lock();   // lock the mutex to access deq_output
    deq_output.push_front(w);   // push the task
    lck.unlock();   // unlock the mutex
    cv_boss.notify_one();   // wake up the boss if it is waiting
  }
}

int main(int argc, char *argv[]) {
  //argv[2~5] : (i, j, k, l)
  int iteration = atoi(argv[1]);
  int num_b = atoi(argv[2]), num_r = atoi(argv[3]), num_eb = atoi(argv[4]), num_er = atoi(argv[5]);
  bool is_opponent = false;
  if(string(argv[6]) == "e") is_opponent = true;
  else assert(string(argv[6]) == "s");

  string filename_self = "data/db/" + base[0] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_enemy = "data/db/" + base[1] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_self_opp = "data/db/" + base[0] + '_' + to_string(num_eb) + '-' + to_string(num_er) + '-' + to_string(num_b) + '-' + to_string(num_r) + ".bin";
  string filename_enemy_opp = "data/db/" + base[1] + '_' + to_string(num_eb) + '-' + to_string(num_er) + '-' + to_string(num_b) + '-' + to_string(num_r) + ".bin";
  string filename_enemy_cap_b = "data/db/" + base[1] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb - 1) + '-' + to_string(num_er) + ".bin";
  string filename_enemy_cap_r = "data/db/" + base[1] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er - 1) + ".bin";
  string filename_self_cap_b = "data/db/" + base[0] + '_' + to_string(num_b - 1) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_self_cap_r = "data/db/" + base[0] + '_' + to_string(num_b) + '-' + to_string(num_r - 1) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_enemy_opp_cap_b = "data/db/" + base[1] + '_' + to_string(num_eb) + '-' + to_string(num_er) + '-' + to_string(num_b - 1) + '-' + to_string(num_r) + ".bin";
  string filename_enemy_opp_cap_r = "data/db/" + base[1] + '_' + to_string(num_eb) + '-' + to_string(num_er) + '-' + to_string(num_b) + '-' + to_string(num_r - 1) + ".bin";
  string filename_self_opp_cap_b = "data/db/" + base[0] + '_' + to_string(num_eb - 1) + '-' + to_string(num_er) + '-' + to_string(num_b) + '-' + to_string(num_r) + ".bin";
  string filename_self_opp_cap_r = "data/db/" + base[0] + '_' + to_string(num_eb) + '-' + to_string(num_er - 1) + '-' + to_string(num_b) + '-' + to_string(num_r) + ".bin";
  
  unsigned long long int placement = placement_count[num_b][num_r][num_eb + num_er];
  unsigned long long int placement_opp = placement_count[num_eb][num_er][num_b + num_r];
  unsigned long long int placement_self_cap_b = placement_count[num_b - 1][num_r][num_eb + num_er];
  unsigned long long int placement_self_cap_r = placement_count[num_b][num_r - 1][num_eb + num_er];
  unsigned long long int placement_enemy_cap = placement_count[num_b][num_r][num_eb + num_er - 1];
  unsigned long long int placement_self_opp_cap_b = placement_count[num_eb - 1][num_er][num_b + num_r];
  unsigned long long int placement_self_opp_cap_r = placement_count[num_eb][num_er - 1][num_b + num_r];
  unsigned long long int placement_enemy_opp_cap = placement_count[num_eb][num_er][num_b + num_r - 1];
  
  unique_ptr<ZDD> zdd = make_unique<ZDD>(num_b, num_r, num_eb + num_er);
  unique_ptr<ZDD> zdd_opp = make_unique<ZDD>(num_eb, num_er, num_b + num_r);
  unique_ptr<ZDD> zdd_enemy_cap = make_unique<ZDD>(num_b, num_r, num_eb + num_er - 1);
  unique_ptr<ZDD> zdd_self_cap_b = make_unique<ZDD>(num_b - 1, num_r, num_eb + num_er);
  unique_ptr<ZDD> zdd_self_cap_r = make_unique<ZDD>(num_b, num_r - 1, num_eb + num_er);
  unique_ptr<ZDD> zdd_enemy_opp_cap = make_unique<ZDD>(num_eb, num_er, num_b + num_r - 1);
  unique_ptr<ZDD> zdd_self_opp_cap_b = make_unique<ZDD>(num_eb - 1, num_er, num_b + num_r);
  unique_ptr<ZDD> zdd_self_opp_cap_r = make_unique<ZDD>(num_eb, num_er - 1, num_b + num_r);

  int self_iteration, enemy_iteration, self_iteration_opp, enemy_iteration_opp;
  if(iteration % 2 == 1) {
    if(string(argv[6]) == "s") {
      self_iteration = iteration - 2;
    } else {
      assert(string(argv[6]) == "e");
      self_iteration = iteration;
    }
    enemy_iteration = iteration - 1;
    self_iteration_opp = iteration - 2;
    enemy_iteration_opp = iteration - 1;
  } else {
    if(string(argv[6]) == "s") {
      enemy_iteration = iteration - 2;
    } else {
      assert(string(argv[6]) == "e");
      enemy_iteration = iteration;
    }
    self_iteration = iteration - 1;
    self_iteration_opp = iteration - 1;
    enemy_iteration_opp = iteration - 2;
  }
  
  Table table_self(self_iteration, filename_self.c_str(), 2, placement);
  Table table_enemy(enemy_iteration, filename_enemy.c_str(), 2, placement);
  Table table_self_opp(self_iteration_opp, filename_self_opp.c_str(), 2, placement_opp);
  Table table_enemy_opp(enemy_iteration_opp, filename_enemy_opp.c_str(), 2, placement_opp);
  Table table_self_cap_b(0, filename_self_cap_b.c_str(), 2, placement_self_cap_b);
  Table table_self_cap_r(0, filename_self_cap_r.c_str(), 2, placement_self_cap_r);
  Table table_enemy_cap_b(0, filename_enemy_cap_b.c_str(), 2, placement_enemy_cap);
  Table table_enemy_cap_r(0, filename_enemy_cap_r.c_str(), 2, placement_enemy_cap);
  Table table_self_opp_cap_b(0, filename_self_opp_cap_b.c_str(), 2, placement_self_opp_cap_b);
  Table table_self_opp_cap_r(0, filename_self_opp_cap_r.c_str(), 2, placement_self_opp_cap_r);
  Table table_enemy_opp_cap_b(0, filename_enemy_opp_cap_b.c_str(), 2, placement_enemy_opp_cap);
  Table table_enemy_opp_cap_r(0, filename_enemy_opp_cap_r.c_str(), 2, placement_enemy_opp_cap);
    
  while(true) {
    if(num_b == num_eb && num_r == num_er) {
      std::cout << "iter > " << iteration << ": (" << num_b << ", " << num_r << ", " << num_eb << ", " << num_er << ")" << endl;
      
      // flag indicating whether the worker has finished its work
      // always set to false before creating the boss
      flag_worker_quit = false;
      
      if(iteration % 2 == 1) {
	thread th_boss(boss, iteration, num_b, num_r, num_eb, num_er,
		       ref(table_self), filename_self.c_str(),
		       cref(table_enemy), cref(table_enemy_cap_b), cref(table_enemy_cap_r),
		       cref(table_enemy));   // create the boss thread
	
	thread th_worker[nworker];   // create the worker threads
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid] = thread(worker, iteration, num_b, num_r, num_eb, num_er,
				       cref(*zdd), cref(*zdd_enemy_cap), cref(*zdd_enemy_cap),
				       cref(*zdd));
	}
	
	// cleanup
	th_boss.join(); 
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid].join();
	}
      } else {
	thread th_boss(boss, iteration, num_b, num_r, num_eb, num_er,
		       ref(table_enemy), filename_enemy.c_str(),
		       cref(table_self), cref(table_self_cap_b), cref(table_self_cap_r),
		       cref(table_self));   // create the boss thread
	
	thread th_worker[nworker];   // create the worker threads
	for(int workerid = 0; workerid < nworker; workerid++) {
	  th_worker[workerid] = thread(worker, iteration, num_b, num_r, num_eb, num_er,
				       cref(*zdd), cref(*zdd_self_cap_b), cref(*zdd_self_cap_r),
				       cref(*zdd));
	}
	
	// cleanup
	th_boss.join(); 
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid].join();
	}
      }
      
      if(count_changes == 0) {
	cout << "analysis finished" << endl;
	break;
      }
    } else {
      assert(num_b != num_eb || num_r != num_er);

      if(!is_opponent) {
	// First, process (num_b, num_r, num_eb, num_er)
	std::cout << "iter > " << iteration << ": (" << num_b << ", " << num_r << ", " << num_eb << ", " << num_er << ")" << endl;
	
	// flag indicating whether the worker has finished its work
	// always set to false before creating the boss
	flag_worker_quit = false;
	
	if(iteration % 2 == 1) {
	  thread th_boss(boss, iteration, num_b, num_r, num_eb, num_er,
			 ref(table_self), filename_self.c_str(),
			 cref(table_enemy), cref(table_enemy_cap_b), cref(table_enemy_cap_r),
			 cref(table_enemy_opp));   // create the boss thread
	  
	  thread th_worker[nworker];   // create the worker threads
	  for(int workerid = 0; workerid < nworker; workerid++){
	    th_worker[workerid] = thread(worker, iteration, num_b, num_r, num_eb, num_er,
					 cref(*zdd), cref(*zdd_enemy_cap), cref(*zdd_enemy_cap),
					 cref(*zdd_opp));   //ここでworker()を呼び出す
	  }
	  
	  // cleanup
	  th_boss.join(); 
	  for(int workerid = 0; workerid < nworker; workerid++){
	    th_worker[workerid].join();
	  }
	} else {
	  thread th_boss(boss, iteration, num_b, num_r, num_eb, num_er,
			 ref(table_enemy), filename_enemy.c_str(),
			 cref(table_self), cref(table_self_cap_b), cref(table_self_cap_r),
			 cref(table_self_opp));   // create the boss thread
	  
	  thread th_worker[nworker];   // create the worker threads
	  for(int workerid = 0; workerid < nworker; workerid++) {
	    th_worker[workerid] = thread(worker, iteration, num_b, num_r, num_eb, num_er,
					 cref(*zdd), cref(*zdd_self_cap_b), cref(*zdd_self_cap_r),
					 cref(*zdd_opp));
	  }
	
	  // cleanup
	  th_boss.join(); 
	  for(int workerid = 0; workerid < nworker; workerid++){
	    th_worker[workerid].join();
	  }
	}
      }

      // Next, process (num_eb, num_er, num_b, num_r)
      std::cout << "iter > " << iteration << ": (" << num_eb << ", " << num_er << ", " << num_b << ", " << num_r << ")" << endl;
      int count_changes_copy = count_changes;
      
      // flag indicating whether the worker has finished its work
      // always set to false before creating the boss
      flag_worker_quit = false;
      
      if(iteration % 2 == 1) {
	thread th_boss(boss, iteration, num_eb, num_er, num_b, num_r,
		       ref(table_self_opp), filename_self_opp.c_str(),
		       cref(table_enemy_opp), cref(table_enemy_opp_cap_b), cref(table_enemy_opp_cap_r),
		       cref(table_enemy));   // create the boss thread
	
	thread th_worker[nworker];   // create the worker threads
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid] = thread(worker, iteration, num_eb, num_er, num_b, num_r,
				       cref(*zdd_opp), cref(*zdd_enemy_opp_cap), cref(*zdd_enemy_opp_cap),
				       cref(*zdd));   //ここでworker()を呼び出す
	}
	
	// cleanup
	th_boss.join(); 
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid].join();
	}
      } else {
	thread th_boss(boss, iteration, num_eb, num_er, num_b, num_r,
		       ref(table_enemy_opp), filename_enemy_opp.c_str(),
		       cref(table_self_opp), cref(table_self_opp_cap_b), cref(table_self_opp_cap_r),
		       cref(table_self));   // create the boss thread
	
	thread th_worker[nworker];   // create tge worker threads
	for(int workerid = 0; workerid < nworker; workerid++) {
	  th_worker[workerid] = thread(worker, iteration, num_eb, num_er, num_b, num_r,
				       cref(*zdd_opp), cref(*zdd_self_opp_cap_b), cref(*zdd_self_opp_cap_r),
				       cref(*zdd));
	}
	
	// cleanup
	th_boss.join(); 
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid].join();
	}
      }
      
      if(count_changes == 0 && count_changes_copy == 0) {
	cout << "analysis finished" << endl;
	break;
      }

      is_opponent = false;
    }
    iteration++;
  }

  return 0;
}
  
// g++ -std=c++14 -O2 -Wall -pthread src/analysis/main.cpp src/common/node.cpp src/common/zdd_geister.cpp src/common/posi_geister.cpp src/common/table.cpp -o bin/main 2>&1 
// ./bin/main 1 1 1 1 1 s 2>&1 &


// make
// ./scripts/run_main.sh &
