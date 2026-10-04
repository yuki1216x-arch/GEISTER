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

constexpr int nworker = 7;   // number of parallel threads
constexpr int deq_input_size = 2048;    
constexpr int deq_output_size = 256;
constexpr int max_legal_num = 32;
constexpr int max_belief_state = 70;

condition_variable cv_boss;   // a condition variable is also one of the synchronization mechanisms provided by POSIX threads
condition_variable cv_worker;   
mutex mtx;  
// POSIX threading
// name of the POSIX interface
// use a POSIX thread mutex here

bool flag_worker_quit;   // flag indicating whether all configurations have been analyzed once
int flag = 0;
unsigned long long int search_id = 0ULL;

// unsigned long long int keiro[422], length = 0/*, x*/; //-----------------------------

// enum {b000 = 0, b001, b010, b011, b100, b101, b110, b111 };

// class representing a task
class Work {
private:
  unsigned long long int m_id;   // assigned ID
  int m_nchild;   // number of legal moves for m_id
  // after applying the move and configuration
  int m_captured_piece_type[max_legal_num];   // color of the captured piece
  long long int m_array_id[max_legal_num];   // ID of the non-terminal configuration

public:
  Work() noexcept {}
  Work(unsigned long long int id) noexcept : m_id(id) {}
  void set_id(unsigned long long int id) noexcept { m_id = id; }
  void set(int nchild, int captured_piece_type[max_legal_num], long long int array_id[max_legal_num]) noexcept {
    assert(nchild > 0 && nchild <= max_legal_num);
    m_nchild = nchild;
    for(int i = 0; i < m_nchild; i++) {
      m_captured_piece_type[i] = captured_piece_type[i];
      m_array_id[i] = array_id[i];
    }
  }
  unsigned long long int get_id() const noexcept { return m_id; }
  int get(int captured_piece_type[max_legal_num], long long int array_id[max_legal_num]) const noexcept {
    assert(m_nchild > 0 && m_nchild <= max_legal_num);
    for(int i = 0; i < m_nchild; i++) {
      captured_piece_type[i] = m_captured_piece_type[i];
      array_id[i] = m_array_id[i];
    }
    return m_nchild;
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
	  
unsigned long long int nwin = 0, nlose = 0;   // number of newly assigned labels of each type (for verification)

unsigned long long int count_changes = 0ULL;   // number of updates (stop the search when it reaches zero)

deque<Work *> deq_input;   // the boss stores tasks here, and workers retrieve them to find legal moves
deque<Work *> deq_output;   // workers store the computed IDs here, and the boss retrieves them and writes them to the table

// boss function
static void boss(int iter, int num_b, int num_r, int num_eb, int num_er,
		 Table& parent_table, const char* write_filename,
		 const Table& child_table, const Table& child_table_cap_b, const Table& child_table_cap_r) noexcept {
  std::cout << "boss" << endl;

  nwin = 0, nlose = 0;
  count_changes = 0ULL;
  
  unsigned long long int max_placement = placement_count[num_b][num_r][num_eb + num_er];
  unsigned long long int count_input = 0ULL;   // number of configurations checked to determine whether they should be analyzed in this iteration
  unsigned long long int count_output = 0ULL;   // number of configurations whose processing has been completed
  
  int nstack_work_idle = deq_input_size + deq_output_size + nworker;   // number of tasks that can currently be processed
  Work* stack_work_idle[nstack_work_idle];   // array for storing tasks
  for(int i = 0; i < nstack_work_idle; i++) stack_work_idle[i] = new Work;   // initialize the array with empty tasks
  
  while(true) {   // repeat until there are no tasks left
    unique_lock<mutex> lck(mtx);   // Lock the mutex; mtx is automatically unlocked when the unique_lock instance lck is destroyed
    cv_boss.wait(lck, [&](){   // wait (unique_lock instance, lambda expression capturing by reference)
			return (((deq_input.size() < deq_input_size) && (deq_output.size() < deq_output_size) && (count_input < max_placement))
				|| (0 < deq_output.size())); });
    // wait until deq_input and deq_output have enough free space
    // wait blocks while the condition is false and returns when the condition becomes true
    // condition: the return value of the function passed as the second argument
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

      while(count_input < max_placement && (before_value = parent_table.get(count_input)) != v_unknown) {
	// keep incrementing the ID until an unknown label is found
	count_input++;
	count_output++;
      }
      if(count_input >= max_placement) {
	lck.unlock();
	continue;
      }
                
      assert(nstack_work_idle >= 1);
      Work *pw = stack_work_idle[ --nstack_work_idle ];   // retrieve a task to assign from stack_work_idle
      pw->set_id(count_input);   // set the configuration ID and the value assigned by the previous analysis for the worker
      deq_input.push_front(pw);   // add a task
      lck.unlock();
      count_input++;
      
      if(count_input % 1000000000ULL == 0ULL) std::cout << "count_input: " << count_input << ", nwin: " << nwin  << ", nlose: " << nlose << endl;
      
      cv_worker.notify_one();   // wake up one worker thread
    } else {   // if tasks have accumulated (Step 3)
      if (0 < deq_output.size()) {
	assert(0 < deq_output.size());
	deque<Work *> deq_tmp; // use swap
	swap(deq_tmp, deq_output);
	
	// empty deq_output
	deq_output.clear();
	lck.unlock();   // swap with deq_tmp, which is not accessed by other threads, to release the lock sooner
	count_output += deq_tmp.size();
	for (unsigned int workid = 0; workid < deq_tmp.size(); workid++) {
	  unsigned long long int id = deq_tmp[workid]->get_id();   // retrieve the assigned ID from the task
	  
	  int captured_piece_type[max_legal_num];
	  long long int array_id[max_legal_num];
	  int nchild = deq_tmp[workid]->get(captured_piece_type, array_id);   // retrieve the number of children, their IDs, and the types of captured pieces
	    
	  assert(nchild > 0);   // Invalid: there should be at least one child
	  int win_plan_num = 0;   // number of forced-win strategies
	  int lose_plan_num = 0;   // number of exist-loss strategies
	  bool already_decided = false;
	  for(int j = 0; j < nchild; j++) {
	    long long int num_of_haiti = array_id[j];
	    if(num_of_haiti >= 0) {   // not terminal; retrieve the value from the database
	      int child_val;
	      int which_table = captured_piece_type[j];
	      // no piece captured
	      if(which_table == 0) child_val = child_table.get((unsigned long long int)num_of_haiti);
	      // blue piece captured
	      else if(which_table == 1) child_val = child_table_cap_b.get((unsigned long long int)num_of_haiti);
	      // red piece captured
	      else {
		assert(which_table == 2);
		child_val = child_table_cap_r.get((unsigned long long int)num_of_haiti);
	      }
	      if(child_val == v_win) win_plan_num++;
	      else if(child_val == v_lose) lose_plan_num++;
	      else {
		assert(child_val == v_unknown);
	      }
	    } else {
	      if(num_of_haiti == -1) {
		// player1 wins
		win_plan_num++;
	      } else if(num_of_haiti == -2) {
		// player2 loses
		lose_plan_num++;
	      } else {
		// Do not count this move because it is illegal
	      }
	    }

	    if(iter % 2 == 1) {
	      if(win_plan_num >= 1) {
		parent_table.set(id, v_win);
		nwin++;
		count_changes++;
		already_decided = true;
		break;
	      }
	    } else {
	      if(lose_plan_num >= 1) {
		parent_table.set(id, v_lose);
		nlose++;
		count_changes++;
		already_decided = true;
		break;
	      }
	    }
	  }
	  
	  // after checking all legal moves
	  if(!already_decided) {
	    if(iter % 2 == 1) {
	      if(lose_plan_num == nchild) {  // lose
		parent_table.set(id, v_lose);
		nlose++;
		count_changes++;
	      }
	    } else {
	      if(win_plan_num == nchild) { // win
		parent_table.set(id, v_win);
		nwin++;
		count_changes++;
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
		   const ZDD& zdd_parent, const ZDD& zdd_child_cap_b, const ZDD& zdd_child_cap_r) noexcept {
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
    Posi p;
    p.make_posi(id, zdd_parent, num_b, num_r, num_eb, num_er);   // create the position for a given ID
    Action actions[max_legal_num];
    int nchild = p.compute_actions(actions, iter);   // enumerate legal moves for the position
    assert(nchild > 0 && nchild < max_legal_num);
      
    int captured_piece_type[max_legal_num] = {};
    long long int array_id[max_legal_num];
      
    for(int i = 0; i < nchild; i++) {
      int board_check = p.make_action(actions[i]);   // apply the legal move
      if(board_check >= 0) {   // the game is not decided directly
	if(board_check == 0) {
	  array_id[i] = p.getzddnum(zdd_parent);
	} else if(board_check == 1) {
	  array_id[i] = p.getzddnum(zdd_child_cap_b);
	  captured_piece_type[i] = 1;
	} else {
	  assert(board_check == 2);
	  array_id[i] = p.getzddnum(zdd_child_cap_r);
	  captured_piece_type[i] = 2;
	}
      } else {
	array_id[i] = board_check;
      }
      p.undo_action();
    }
    w->set(nchild, captured_piece_type, array_id);   // set the child ID, captured piece, and other information
         
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

  string filename_self = "data/db_purple/" + base[0] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_enemy = "data/db_purple/" + base[1] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_enemy_cap_b = "data/db_purple/" + base[1] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb - 1) + '-' + to_string(num_er) + ".bin";
  string filename_enemy_cap_r = "data/db_purple/" + base[1] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er - 1) + ".bin";
  string filename_self_cap_b = "data/db_purple/" + base[0] + '_' + to_string(num_b - 1) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_self_cap_r = "data/db_purple/" + base[0] + '_' + to_string(num_b) + '-' + to_string(num_r - 1) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  
  unsigned long long int placement = placement_count[num_b][num_r][num_eb + num_er];
  unsigned long long int placement_self_cap_b = placement_count[num_b - 1][num_r][num_eb + num_er];
  unsigned long long int placement_self_cap_r = placement_count[num_b][num_r - 1][num_eb + num_er];
  unsigned long long int placement_enemy_cap = placement_count[num_b][num_r][num_eb + num_er - 1];
  
  unique_ptr<ZDD> zdd = make_unique<ZDD>(num_b, num_r, num_eb + num_er);
  unique_ptr<ZDD> zdd_enemy_cap = make_unique<ZDD>(num_b, num_r, num_eb + num_er - 1);
  unique_ptr<ZDD> zdd_self_cap_b = make_unique<ZDD>(num_b - 1, num_r, num_eb + num_er);
  unique_ptr<ZDD> zdd_self_cap_r = make_unique<ZDD>(num_b, num_r - 1, num_eb + num_er);

  int self_iteration, enemy_iteration;
  if(iteration % 2 == 1) {
    self_iteration = iteration - 2;
    enemy_iteration = iteration - 1;
  } else {
    assert(iteration % 2 == 0);
    self_iteration = iteration - 1;
    enemy_iteration = iteration - 2;
  }
  
  Table table_self(self_iteration, filename_self.c_str(), 2, placement);
  Table table_enemy(enemy_iteration, filename_enemy.c_str(), 2, placement);
  Table table_self_cap_b(0, filename_self_cap_b.c_str(), 2, placement_self_cap_b);
  Table table_self_cap_r(0, filename_self_cap_r.c_str(), 2, placement_self_cap_r);
  Table table_enemy_cap_b(0, filename_enemy_cap_b.c_str(), 2, placement_enemy_cap);
  Table table_enemy_cap_r(0, filename_enemy_cap_r.c_str(), 2, placement_enemy_cap);
    
  while(true) {
    iteration++;
    std::cout << "iter > " << iteration << ": (" << num_b << ", " << num_r << ", " << num_eb << ", " << num_er << ")" << endl;
      
    // flag indicating whether the worker has finished its work
    // always set to false before creating the boss
    flag_worker_quit = false;
    
    if(iteration % 2 == 1) {
      thread th_boss(boss, iteration, num_b, num_r, num_eb, num_er,
		     ref(table_self), filename_self.c_str(),
		     cref(table_enemy), cref(table_enemy_cap_b), cref(table_enemy_cap_r));   // create the boss thread
      
      thread th_worker[nworker];   // create the worker threads
      for(int workerid = 0; workerid < nworker; workerid++){
	th_worker[workerid] = thread(worker, iteration, num_b, num_r, num_eb, num_er,
				     cref(*zdd), cref(*zdd_enemy_cap), cref(*zdd_enemy_cap));
      }
	
      // cleanup
      th_boss.join(); 
      for(int workerid = 0; workerid < nworker; workerid++){
	th_worker[workerid].join();
      }
    } else {
      thread th_boss(boss, iteration, num_b, num_r, num_eb, num_er,
		     ref(table_enemy), filename_enemy.c_str(),
		     cref(table_self), cref(table_self_cap_b), cref(table_self_cap_r));   // create the boss thread
	
      thread th_worker[nworker];   // create the worker threads
      for(int workerid = 0; workerid < nworker; workerid++) {
	th_worker[workerid] = thread(worker, iteration, num_b, num_r, num_eb, num_er,
				     cref(*zdd), cref(*zdd_self_cap_b), cref(*zdd_self_cap_r));
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
  }

  return 0;
}
  
// g++ -std=c++14 -O2 -Wall -pthread src/analysis/main_purple.cpp src/common/node.cpp src/common/zdd_geister.cpp src/common/posi_geister.cpp src/common/table.cpp -o bin/main_purple 2>&1
// ./bin/main_purple 1 1 1 1 1 2>&1 &


// make
// ./scripts/run_main_purple.sh &
