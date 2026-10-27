// Construct Geister endgame databases by retrograde analysis.
// Labels: forced win (FW), can lose (CL), and forced loss (FL).
// Each database entry uses 2 bits.
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
#include "../common/position_geister.hpp"
#include "../common/zdd_geister.hpp"

using std::ref;
using std::cref;

using namespace std;

constexpr int worker_count = 7;  // number of worker threads (excluding the coordinator)
constexpr int input_queue_capacity = 2048;    
constexpr int output_queue_capacity = 256;
constexpr int max_legal_moves = 32;
constexpr int max_compatible_configurations = 70;

condition_variable coordinator_cv;   // singals completed worker tasks
condition_variable worker_cv;   // signals tasks or shutdown
mutex queue_mutex;   // protects task queues and the shutdown flag

bool shutdown_requested;   // worker shutdown request

// Reusable task: configuration ID, initial label, and computed child information.
class AnalysisTask {
private:
  unsigned long long int m_configuration_id;   // configuration ID
  unsigned int m_initial_label;   // label before this task is processed
  int m_legal_move_count;   // number of legal moves
  int m_compatible_configuration_count;   // compatible third-party configuration count
  // Child information indexed by [move][third-party configuration].
  // Capture codes: 0 = none, 1 = blue, 2 = red.
  int m_captured_piece_type[max_legal_moves][max_compatible_configurations];
  // Nonnegative values are child IDs; -1 and -2 represent terminal wins and losses.
  long long int m_child_results[max_legal_moves][max_compatible_configurations];
  int m_loss_captured_piece_types[max_compatible_configurations][max_legal_moves];
  long long int m_loss_child_results[max_compatible_configurations][max_legal_moves];
  long long int m_reversed_configuration_ids[max_compatible_configurations];

public:
  AnalysisTask() noexcept {}
  AnalysisTask(unsigned long long int configuration_id) noexcept : m_configuration_id(configuration_id) {}
  void set_configuration(unsigned long long int configuration_id, unsigned int label) noexcept {
    m_configuration_id = configuration_id;
    m_initial_label = label;
  }
  void set_move_children(int legal_move_count, int compatible_configuration_count, int captured_piece_type[max_legal_moves][max_compatible_configurations], long long int child_results[max_legal_moves][max_compatible_configurations]) noexcept {
    assert(legal_move_count > 0 && legal_move_count <= max_legal_moves);
    assert(compatible_configuration_count > 0 && compatible_configuration_count <= max_compatible_configurations);
    m_legal_move_count = legal_move_count;
    m_compatible_configuration_count = compatible_configuration_count;
    for(int i = 0; i < m_legal_move_count; i++) {
      for(int j = 0; j < m_compatible_configuration_count; j++) {
	m_captured_piece_type[i][j] = captured_piece_type[i][j];
	m_child_results[i][j] = child_results[i][j];
      }
    }
  }
  // Store reversed IDs for forced-loss checks on even iterations.
  void set_loss_children(int compatible_configuration_count, long long int reversed_configuration_ids[max_compatible_configurations]) noexcept {
    assert(compatible_configuration_count > 0 && compatible_configuration_count <= max_compatible_configurations);
    m_compatible_configuration_count = compatible_configuration_count;
    for(int i = 0; i < m_compatible_configuration_count; i++) {
      m_reversed_configuration_ids[i] = reversed_configuration_ids[i];
    }
  }
  // Store children indexed by [third-party configuration][move] for forced-loss checks.
  void set_loss_children(int legal_move_count, int compatible_configuration_count, int loss_captured_piece_types[max_compatible_configurations][max_legal_moves], long long int loss_child_results[max_compatible_configurations][max_legal_moves]) noexcept {
    assert(legal_move_count > 0 && legal_move_count <= max_legal_moves);
    assert(compatible_configuration_count > 0 && compatible_configuration_count <= max_compatible_configurations);
    m_legal_move_count = legal_move_count;
    m_compatible_configuration_count = compatible_configuration_count;
    for(int i = 0; i < m_compatible_configuration_count; i++) {
      for(int j = 0; j < m_legal_move_count; j++) {
	m_loss_captured_piece_types[i][j] = loss_captured_piece_types[i][j];
	m_loss_child_results[i][j] = loss_child_results[i][j];
      }
    }
  }
  unsigned long long int get_configuration_id() const noexcept { return m_configuration_id; }
  unsigned int get_initial_label() const noexcept { return m_initial_label; }
  void get_move_children(int& legal_move_count, int& compatible_configuration_count, int captured_piece_type[max_legal_moves][max_compatible_configurations], long long int child_results[max_legal_moves][max_compatible_configurations]) const noexcept {
    assert(m_legal_move_count > 0 && m_legal_move_count <= max_legal_moves);
    assert(m_compatible_configuration_count > 0 && m_compatible_configuration_count <= max_compatible_configurations);
    legal_move_count = m_legal_move_count;
    compatible_configuration_count = m_compatible_configuration_count;
    for(int i = 0; i < m_legal_move_count; i++) {
      for(int j = 0; j < m_compatible_configuration_count; j++) {
	captured_piece_type[i][j] = m_captured_piece_type[i][j];
	child_results[i][j] = m_child_results[i][j];
      }
    }
  }
  void get_loss_children(int& compatible_configuration_count, long long int reversed_configuration_ids[max_compatible_configurations]) const noexcept {
    assert(m_compatible_configuration_count > 0 && m_compatible_configuration_count <= max_compatible_configurations);
    compatible_configuration_count = m_compatible_configuration_count;
    for(int i = 0; i < m_compatible_configuration_count; i++) {
      reversed_configuration_ids[i] = m_reversed_configuration_ids[i];
    }
  }
  void get_loss_children(int& legal_move_count, int& compatible_configuration_count, int loss_captured_piece_types[max_compatible_configurations][max_legal_moves], long long int loss_child_results[max_compatible_configurations][max_legal_moves]) const noexcept {
    assert(m_legal_move_count > 0 && m_legal_move_count <= max_legal_moves);
    assert(m_compatible_configuration_count > 0 && m_compatible_configuration_count <= max_compatible_configurations);
    legal_move_count = m_legal_move_count;
    compatible_configuration_count = m_compatible_configuration_count;
    for(int i = 0; i < m_compatible_configuration_count; i++) {
      for(int j = 0; j < m_legal_move_count; j++) {
	loss_captured_piece_types[i][j] = m_loss_captured_piece_types[i][j];
	loss_child_results[i][j] = m_loss_child_results[i][j];
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

const string table_filename_prefixes[2] = {"self_table", "enemy_table"};

// assignments in this pass
unsigned long long int fw_assignment_count = 0, fl_assignment_count = 0, cl_assignment_count = 0;

unsigned long long int label_update_count = 0ULL;   // updates in this pass

// The coordinator queues configurations for workers to expand.
deque<AnalysisTask *> input_queue;
// Workers return child information for the coordinator to assign labels.
deque<AnalysisTask *> output_queue;

// Schedule tasks, assign labels from worker results, and write the database.
static void run_coordinator(int iteration, int self_blue_count, int self_red_count, int opponent_blue_count, int opponent_red_count,
		 Table& parent_table, const char* output_database_path,
		 const Table& child_table, const Table& child_table_after_blue_capture, const Table& child_table_after_red_capture,
		 const Table& child_table_reversed) noexcept {
  cout << "[analysis] Starting configuration scan" << endl;

  fw_assignment_count = 0, fl_assignment_count = 0, cl_assignment_count = 0;
  label_update_count = 0ULL;
  
  unsigned long long int configuration_count = placement_count[self_blue_count][self_red_count][opponent_blue_count + opponent_red_count];
  // number of configuration IDs scanned in this pass
  unsigned long long int scanned_configuration_count = 0ULL;
  // number of completed or skipped configurations
  unsigned long long int completed_configuration_count = 0ULL;

  // current number of reusable tasks
  int available_task_count = input_queue_capacity + output_queue_capacity + worker_count;
  AnalysisTask* available_tasks[available_task_count];   // array for storing tasks
  // initialize the array with empty tasks
  for(int i = 0; i < available_task_count; i++) available_tasks[i] = new AnalysisTask;
  
  while(true) {
    unique_lock<mutex> queue_lock(queue_mutex);
    // Wait for queue capacity or completed results; release the mutex white blocked.
    coordinator_cv.wait(queue_lock, [&](){
      return (((input_queue.size() < input_queue_capacity) && (output_queue.size() < output_queue_capacity) && (scanned_configuration_count < configuration_count))
				|| (0 < output_queue.size())); });

    // Queue the next configuration requiring analysis when both queues have capacity.
    if ((input_queue.size() < input_queue_capacity) && (output_queue.size() < output_queue_capacity) && scanned_configuration_count < configuration_count) {
      // retrieve the label from the retrograde analysis table
      // continue the analysis if the label is unknown or can-lose
      unsigned int initial_label;
      while(scanned_configuration_count < configuration_count && (initial_label = parent_table.get(scanned_configuration_count)) != v_unknown && initial_label != v_can_lose) {
	// keep incrementing the ID until an unknown or can-lose label is found
	scanned_configuration_count++;
	completed_configuration_count++;
      }

      if(scanned_configuration_count >= configuration_count) {
	queue_lock.unlock();
	continue;
      }
      
      assert(available_task_count >= 1);
      // retrieve a task to assign from available_tasks
      AnalysisTask *task = available_tasks[ --available_task_count ];
      // set the configuration ID and the label assigned by the previous analysis for the worker
      task->set_configuration(scanned_configuration_count, initial_label);
      input_queue.push_front(task);
      queue_lock.unlock();
      scanned_configuration_count++;
      
      if(scanned_configuration_count % 1000000000ULL == 0ULL)
	cout << "[progress] scanned_configurations: " << scanned_configuration_count
	     << ", FW_assignments: " << fw_assignment_count
	     << ", CL_assignments: " << cl_assignment_count
	     << ", FL_assignments: " << fl_assignment_count << endl;

      // wake up one worker thread
      worker_cv.notify_one();
    // consume completed worker tasks
    } else {
      if (0 < output_queue.size()) {
	deque<AnalysisTask *> completed_tasks;
	// Take ownership of results so label assignment can proceed outside the lock
	swap(completed_tasks, output_queue);
	
	output_queue.clear();
	// swap with completed_tasks, which is not accessed by other threads, to release the lock sooner
	queue_lock.unlock();
	completed_configuration_count += completed_tasks.size();
	for (unsigned int task_index = 0; task_index < completed_tasks.size(); task_index++) {
	  // retrieve the assigned ID from the task
	  unsigned long long int configuration_id = completed_tasks[task_index]->get_configuration_id();
	  unsigned int label = completed_tasks[task_index]->get_initial_label();
	  
	  if(label == v_unknown) {
	    int legal_move_count, compatible_configuration_count;
	    int captured_piece_type[max_legal_moves][max_compatible_configurations];
	    long long int child_results[max_legal_moves][max_compatible_configurations];
	    // retrieve all information needed to determine forced-win and can-lose labels
	    completed_tasks[task_index]->get_move_children(legal_move_count, compatible_configuration_count, captured_piece_type, child_results);
	    bool label_assigned = false;
	    int action_count = legal_move_count;
	    
	    assert(legal_move_count > 0);
	    int winning_move_count = 0;  // moves satisfying the forced-win condition
	    int can_lose_move_count = 0;  // moves satisfying the can-lose condition
	    for(int j = 0; j < action_count; j++) {
	      int win_count = 0;
	      int loss_count = 0;
	      int can_lose_count = 0;
	      int unknown_count = 0;
	      for(int k = 0; k < compatible_configuration_count; k++) {
		long long int child_result = child_results[j][k];
		// not terminal; retrieve the label from the database
		if(child_result >= 0) {
		  int child_label;
		  int captured_piece = captured_piece_type[j][k];
		  // no piece captured
		  if(captured_piece == 0) child_label = child_table.get((unsigned long long int)child_result);
		  // blue piece captured
		  else if(captured_piece == 1) child_label = child_table_after_blue_capture.get((unsigned long long int)child_result);
		  // red piece captured
		  else {
		    assert(captured_piece == 2);
		    child_label = child_table_after_red_capture.get((unsigned long long int)child_result);
		  }
		  
		  if(child_label == v_win) win_count++;
		  else if(child_label == v_loss) loss_count++;
		  else if(child_label == v_can_lose) can_lose_count++;
		  else {
		    assert(child_label == v_unknown);
		    unknown_count++;
		  }
		} else {
		  if(child_result == -1) {
		    // terminal win for the database perspective
		    win_count++;
		  } else if(child_result == -2) {
		    // terminal loss for the database perspective
		    loss_count++;
		  } else {
		    // Do not count this move because it is illegal
		  }
		}
	      }

	      // forced win
	      if(win_count == compatible_configuration_count) {
		if(iteration % 2 == 1) {
		  if(!label_assigned) {
		    parent_table.set(configuration_id, v_win);
		    fw_assignment_count++, label_update_count++;
		    label_assigned = true;
		    label = v_win;
		  }
		} else {
		  winning_move_count++;
		}
		// can lose
	      } else if(loss_count + can_lose_count >= 1) {
		if(iteration % 2 == 1) {
		  can_lose_move_count++;
		} else {
		  if(!label_assigned) {
		    parent_table.set(configuration_id, v_can_lose);
		    label = v_can_lose;
		    cl_assignment_count++;
		    label_update_count++;
		    label_assigned = true;
		  }
		}
	      }
	    }
	  
	    // after checking all legal moves
	    if(!label_assigned) {
	      if(iteration % 2 == 1) {
		// can lose
		if(can_lose_move_count == action_count) {
		  parent_table.set(configuration_id, v_can_lose);
		  label = v_can_lose;
		  cl_assignment_count++;
		  label_update_count++;
		}
	      } else {
		// forced win
		if(winning_move_count == action_count) {
		  parent_table.set(configuration_id, v_win);
		  fw_assignment_count++, label_update_count++;
		  label = v_win;
		}
	      }
	    }
	  }

	  if(label == v_unknown || label == v_can_lose) {
	    if(iteration % 2 == 1) {
	      int legal_move_count, compatible_configuration_count;
	      int loss_captured_piece_types[max_compatible_configurations][max_legal_moves];
	      long long int loss_child_results[max_compatible_configurations][max_legal_moves];
	      // retrieve all information needed to determine forced-loss labels
	      completed_tasks[task_index]->get_loss_children(legal_move_count, compatible_configuration_count, loss_captured_piece_types, loss_child_results);
	      int action_count = legal_move_count;
	      bool label_assigned = false;
	      // configurations satisfying the forced-loss condition
	      int losing_configuration_count = 0;
	      assert(legal_move_count > 0);
	      for(int j = 0; j < compatible_configuration_count; j++) {
		int win_count = 0;
		int loss_count = 0;
		int can_lose_count = 0;
		int unknown_count = 0;
		for(int k = 0; k < action_count; k++) {
		  long long int child_result = loss_child_results[j][k];

		  // not terminal; retrieve the label from the database
		  if(child_result >= 0) {
		    int child_label;
		    int captured_piece = loss_captured_piece_types[j][k];
		    // no piece captured
		    if(captured_piece == 0) child_label = child_table.get((unsigned long long int)child_result);
		    // blue piece captured
		    else if(captured_piece == 1) child_label = child_table_after_blue_capture.get((unsigned long long int)child_result);
		    // red piece captured
		    else {
		      assert(captured_piece == 2);
		      child_label = child_table_after_red_capture.get((unsigned long long int)child_result);
		    }
		    
		    if(child_label == v_win) win_count++;
		    else if(child_label == v_loss) loss_count++;
		    else if(child_label == v_can_lose) can_lose_count++;
		    else {
		      assert(child_label == v_unknown);
		      unknown_count++;
		    }
		  } else {
		    if(child_result == -1) {
		      // terminal win for the database perspective
		      win_count++;
		    } else if(child_result == -2) {
		      // terminal loss for the database perspective
		      loss_count++;
		    } else {
		      // Do not count this move because it is illegal
		    }
		  }
		}

		// forced loss
		if(loss_count == action_count) {
		  losing_configuration_count++;
		}
	      }
	  
	      // after checking all legal moves
	      if(!label_assigned) {
		// forced loss
		if(losing_configuration_count == compatible_configuration_count) {
		  parent_table.set(configuration_id, v_loss);
		  fl_assignment_count++, label_update_count++;
		  label = v_loss;
		}
	      }
	    } else {
	      assert(iteration % 2 == 0);
	      int compatible_configuration_count;
	      long long int reversed_configuration_ids[max_compatible_configurations];
	      completed_tasks[task_index]->get_loss_children(compatible_configuration_count, reversed_configuration_ids);
	      int win_count = 0;
	      int loss_count = 0;
	      int can_lose_count = 0;
	      int unknown_count = 0;
	      for(int j = 0; j < compatible_configuration_count; j++) {
		long long int child_result = reversed_configuration_ids[j];
		assert(child_result >= 0);
		int child_label = child_table_reversed.get((unsigned long long int)child_result);
		
		if(child_label == v_win) win_count++;
		else if(child_label == v_loss) loss_count++;
		else if(child_label == v_can_lose) can_lose_count++;
		else {
		  assert(child_label == v_unknown);
		  unknown_count++;
		}
	      }

	      if(win_count == compatible_configuration_count) {
		parent_table.set(configuration_id, v_loss);
		fl_assignment_count++, label_update_count++;
		label = v_loss;
	      }
	    }
	  }
	}

	// Return completed tasks to the reusable task pool.
	for(size_t i = 0; i < completed_tasks.size(); i++){
	  assert(available_task_count < 4096);
	  available_tasks[ available_task_count++ ] = completed_tasks[i];
	}
      }

      // End this pass once every configuration has been completed or skipped.
      if(completed_configuration_count >= configuration_count) break;
    }
  }
  
  cout << "[output] Writing database: " << output_database_path << endl;
  {
    TableWriter table_writer(iteration, output_database_path, configuration_count, 2);
    for (unsigned long long int i = 0; i < configuration_count; i++) {
      // retrieve the label of configuration_id(i) and copy it directly
      table_writer.write(parent_table.get(i));
    }
    table_writer.flush();
    table_writer.print_statistics();
  }
  
  {
    unique_lock<mutex> queue_lock(queue_mutex);
    // flag indicating that the work is finished
    shutdown_requested = true;
  }
  worker_cv.notify_all();
}

// Expand queued configurations and return child information without assigning labels.
static void run_worker(int iteration, int self_blue_count, int self_red_count, int opponent_blue_count, int opponent_red_count,
		       const ZDD& zdd_parent, const ZDD& zdd_child_after_blue_capture, const ZDD& zdd_child_after_red_capture,
		   const ZDD& zdd_parent_reversed) noexcept {
  AnalysisTask *task;
  while (true) {
    unique_lock<mutex> queue_lock(queue_mutex);
    // wait releases queue_mutex while blocked and reacquires it before returning.
    worker_cv.wait(queue_lock, [&](){ if(0 < input_queue.size()) return true;
	// wait until a task is available or all tasks have been completed
	return shutdown_requested; });
    // exit if all tasks have been completed
    if(shutdown_requested) break;
    assert(0 < input_queue.size());
    // retrieve a task pointer from input_queue
    task = input_queue.back();
    input_queue.pop_back();
    // unlock the mutex used to prevent concurrent access to input_queue
    queue_lock.unlock();
    
    // set all child IDs or direct outcomes for the assigned configuration ID
    unsigned long long int configuration_id = task->get_configuration_id();
    unsigned int label = task->get_initial_label();
    if(label == v_unknown) {
      Position position;
      // create the position for a given ID
      position.load_configuration(configuration_id, zdd_parent, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count);
      Action actions[max_legal_moves];
      unsigned char compatible_boards[max_compatible_configurations][36];
      // enumerate legal moves for the position
      int legal_move_count = position.generate_legal_actions(actions, iteration);
      // enumerate all possible third-party configurations for the position
      int compatible_configuration_count = position.enumerate_compatible_configurations(compatible_boards);
      assert(legal_move_count > 0 && legal_move_count < max_legal_moves);
      assert(compatible_configuration_count > 0 && compatible_configuration_count <= max_compatible_configurations);
      
      int captured_piece_type[max_legal_moves][max_compatible_configurations] = {};
      long long int child_results[max_legal_moves][max_compatible_configurations];
      
      for(int i = 0; i < legal_move_count; i++) {
	for(int j = 0; j < compatible_configuration_count; j++) {
	  // create a configuration from a third-party perspective
	  position.load_compatible_configuration(compatible_boards, j);
	  int move_result = position.apply_action(actions[i]);
	  // the game is not decided directly
	  if(move_result >= 0) {
	    position.hide_opponent_colors();
	    if(move_result == 0) {
	      child_results[i][j] = position.get_zdd_id(zdd_parent);
	    } else if(move_result == 1) {
	      child_results[i][j] = position.get_zdd_id(zdd_child_after_blue_capture);
	      captured_piece_type[i][j] = 1;
	    } else {
	      assert(move_result == 2);
	      child_results[i][j] = position.get_zdd_id(zdd_child_after_red_capture);
	      captured_piece_type[i][j] = 2;
	    }
	  } else {
	    child_results[i][j] = move_result;
	  }
	  position.undo_action();
	}
      }
      // set the child ID, captured piece, and other information
      task->set_move_children(legal_move_count, compatible_configuration_count, captured_piece_type, child_results);
    }
    
    if(label == v_unknown || label == v_can_lose) {
      Position position;
      // create the position for a given ID
      position.load_configuration(configuration_id, zdd_parent, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count);
      Action actions[max_legal_moves];
      unsigned char compatible_boards[max_compatible_configurations][36];
      // enumerate legal moves for the position
      int legal_move_count = position.generate_legal_actions(actions, iteration);
      // enumerate all possible third-party configurations for the position
      int compatible_configuration_count = position.enumerate_compatible_configurations(compatible_boards);
      assert(legal_move_count > 0 && legal_move_count < max_legal_moves);
      assert(compatible_configuration_count > 0 && compatible_configuration_count <= max_compatible_configurations);

      if(iteration % 2 == 1) {
	int loss_captured_piece_types[max_compatible_configurations][max_legal_moves] = {};
	long long int loss_child_results[max_compatible_configurations][max_legal_moves];
      
	for(int i = 0; i < compatible_configuration_count; i++) {
	  for(int j = 0; j < legal_move_count; j++) {
	    // create a configuration from a third-party perspective
	    position.load_compatible_configuration(compatible_boards, i);
	    int move_result = position.apply_action(actions[j]);
	    // the game is not decided directly
	    if(move_result >= 0) {
	      position.hide_opponent_colors();
	      if(move_result == 0) {
		loss_child_results[i][j] = position.get_zdd_id(zdd_parent);
	      } else if(move_result == 1) {
		loss_child_results[i][j] = position.get_zdd_id(zdd_child_after_blue_capture);
		loss_captured_piece_types[i][j] = 1;
	      } else {
		assert(move_result == 2);
		loss_child_results[i][j] = position.get_zdd_id(zdd_child_after_red_capture);
		loss_captured_piece_types[i][j] = 2;
	      }
	    } else {
	      loss_child_results[i][j] = move_result;
	    }
	    position.undo_action();
	  }
	}
	task->set_loss_children(legal_move_count, compatible_configuration_count, loss_captured_piece_types, loss_child_results);
      } else {
	assert(iteration % 2 == 0);
	long long int reversed_configuration_ids[max_compatible_configurations];

	for(int i = 0; i < compatible_configuration_count; i++) {
	  position.load_compatible_configuration(compatible_boards, i);
	  position.switch_to_opponent_view();
	  reversed_configuration_ids[i] = position.get_zdd_id(zdd_parent_reversed);
	}
	task->set_loss_children(compatible_configuration_count, reversed_configuration_ids);
      }
    }

    // lock the mutex to access output_queue
    queue_lock.lock();
    output_queue.push_front(task);
    queue_lock.unlock();
    // wake up the coordinator if it is waiting
    coordinator_cv.notify_one();
  }
}

int main(int argc, char *argv[]) {
  // forward starts with the given piece counts; reversed starts with self/opponent counts swapped.
  // The iteration parity determines which player's turn is processed.
  if(argc == 2 && string(argv[1]) == "--help") {
    cout << "Usage: " << argv[0]
	 << " ITERATION SELF_BLUE SELF_RED OPPONENT_BLUE OPPONENT_RED forward|reversed" << endl;
    cout << "forward: process the given orientation first; reversed: start with swapped piece counts."
	 << endl;
    cout << "Legacy aliases: s = forward, e = reversed." << endl;
    return 0;
  }
  if(argc != 7) {
    cerr << "Expected six arguments. Use --help for usage." << endl;
    return 1;
  }
  const string initial_orientation = argv[6];
  if(initial_orientation != "forward" && initial_orientation != "reversed"
     && initial_orientation != "s" && initial_orientation != "e") {
    cerr << "Invalid orientation: " << initial_orientation
	 << ". Use forward or reversed (Legacy aliases: s/e)." << endl;
    return 1;
  }
  
  int iteration = atoi(argv[1]);
  int self_blue_count = atoi(argv[2]), self_red_count = atoi(argv[3]), opponent_blue_count = atoi(argv[4]), opponent_red_count = atoi(argv[5]);
  bool start_with_reversed_orientation = initial_orientation == "reversed" || initial_orientation == "e";

  // self/opponent turn identifies who moves; reversed swaps the piece-count viewpoint.
  string self_database_path = "data/db/" + table_filename_prefixes[0] + '_' + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-'
    + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  string opponent_database_path = "data/db/" + table_filename_prefixes[1] + '_' + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-'
    + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  string self_reversed_database_path = "data/db/" + table_filename_prefixes[0] + '_' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + '-'
    + to_string(self_blue_count) + '-' + to_string(self_red_count) + ".bin";
  string opponent_reversed_database_path = "data/db/" + table_filename_prefixes[1] + '_' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + '-'
    + to_string(self_blue_count) + '-' + to_string(self_red_count) + ".bin";
  string opponent_after_blue_capture_database_path = "data/db/" + table_filename_prefixes[1] + '_' + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-'
    + to_string(opponent_blue_count - 1) + '-' + to_string(opponent_red_count) + ".bin";
  string opponent_after_red_capture_database_path = "data/db/" + table_filename_prefixes[1] + '_' + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-'
    + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count - 1) + ".bin";
  string self_after_blue_capture_database_path = "data/db/" + table_filename_prefixes[0] + '_' + to_string(self_blue_count - 1) + '-' + to_string(self_red_count) + '-'
    + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  string self_after_red_capture_database_path = "data/db/" + table_filename_prefixes[0] + '_' + to_string(self_blue_count) + '-' + to_string(self_red_count - 1) + '-'
    + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  string opponent_reversed_after_blue_capture_database_path = "data/db/" + table_filename_prefixes[1] + '_'
    + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + '-' + to_string(self_blue_count - 1) + '-' + to_string(self_red_count) + ".bin";
  string opponent_reversed_after_red_capture_database_path = "data/db/" + table_filename_prefixes[1] + '_'
    + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + '-' + to_string(self_blue_count) + '-' + to_string(self_red_count - 1) + ".bin";
  string self_reversed_after_blue_capture_database_path = "data/db/" + table_filename_prefixes[0] + '_'
    + to_string(opponent_blue_count - 1) + '-' + to_string(opponent_red_count) + '-' + to_string(self_blue_count) + '-' + to_string(self_red_count) + ".bin";
  string self_reversed_after_red_capture_database_path = "data/db/" + table_filename_prefixes[0] + '_'
    + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count - 1) + '-' + to_string(self_blue_count) + '-' + to_string(self_red_count) + ".bin";
  
  unsigned long long int configuration_count = placement_count[self_blue_count][self_red_count][opponent_blue_count + opponent_red_count];
  unsigned long long int configuration_count_reversed = placement_count[opponent_blue_count][opponent_red_count][self_blue_count + self_red_count];
  unsigned long long int configuration_count_self_after_blue_capture = placement_count[self_blue_count - 1][self_red_count][opponent_blue_count + opponent_red_count];
  unsigned long long int configuration_count_self_after_red_capture = placement_count[self_blue_count][self_red_count - 1][opponent_blue_count + opponent_red_count];
  unsigned long long int configuration_count_opponent_after_capture = placement_count[self_blue_count][self_red_count][opponent_blue_count + opponent_red_count - 1];
  unsigned long long int configuration_count_self_reversed_after_blue_capture = placement_count[opponent_blue_count - 1][opponent_red_count][self_blue_count + self_red_count];
  unsigned long long int configuration_count_self_reversed_after_red_capture = placement_count[opponent_blue_count][opponent_red_count - 1][self_blue_count + self_red_count];
  unsigned long long int configuration_count_opponent_reversed_after_capture = placement_count[opponent_blue_count][opponent_red_count][self_blue_count + self_red_count - 1];
  
  unique_ptr<ZDD> zdd = make_unique<ZDD>(self_blue_count, self_red_count, opponent_blue_count + opponent_red_count);
  unique_ptr<ZDD> zdd_reversed = make_unique<ZDD>(opponent_blue_count, opponent_red_count, self_blue_count + self_red_count);
  unique_ptr<ZDD> zdd_opponent_after_capture = make_unique<ZDD>(self_blue_count, self_red_count, opponent_blue_count + opponent_red_count - 1);
  unique_ptr<ZDD> zdd_self_after_blue_capture = make_unique<ZDD>(self_blue_count - 1, self_red_count, opponent_blue_count + opponent_red_count);
  unique_ptr<ZDD> zdd_self_after_red_capture = make_unique<ZDD>(self_blue_count, self_red_count - 1, opponent_blue_count + opponent_red_count);
  unique_ptr<ZDD> zdd_opponent_reversed_after_capture = make_unique<ZDD>(opponent_blue_count, opponent_red_count, self_blue_count + self_red_count - 1);
  unique_ptr<ZDD> zdd_self_reversed_after_blue_capture = make_unique<ZDD>(opponent_blue_count - 1, opponent_red_count, self_blue_count + self_red_count);
  unique_ptr<ZDD> zdd_self_reversed_after_red_capture = make_unique<ZDD>(opponent_blue_count, opponent_red_count - 1, self_blue_count + self_red_count);

  int self_turn_iteration, opponent_turn_iteration, self_turn_iteration_reversed, opponent_turn_iteration_reversed;
  if(iteration % 2 == 1) {
    if(!start_with_reversed_orientation) {
      self_turn_iteration = iteration - 2;
    } else {
      self_turn_iteration = iteration;
    }
    opponent_turn_iteration = iteration - 1;
    self_turn_iteration_reversed = iteration - 2;
    opponent_turn_iteration_reversed = iteration - 1;
  } else {
    if(!start_with_reversed_orientation) {
      opponent_turn_iteration = iteration - 2;
    } else {
      opponent_turn_iteration = iteration;
    }
    self_turn_iteration = iteration - 1;
    self_turn_iteration_reversed = iteration - 1;
    opponent_turn_iteration_reversed = iteration - 2;
  }
  
  Table self_turn_table(self_turn_iteration, self_database_path.c_str(), 2, configuration_count);
  Table opponent_turn_table(opponent_turn_iteration, opponent_database_path.c_str(), 2, configuration_count);
  Table self_turn_table_reversed(self_turn_iteration_reversed, self_reversed_database_path.c_str(), 2, configuration_count_reversed);
  Table opponent_turn_table_reversed(opponent_turn_iteration_reversed, opponent_reversed_database_path.c_str(), 2, configuration_count_reversed);
  Table self_turn_table_after_blue_capture(0, self_after_blue_capture_database_path.c_str(), 2, configuration_count_self_after_blue_capture);
  Table self_turn_table_after_red_capture(0, self_after_red_capture_database_path.c_str(), 2, configuration_count_self_after_red_capture);
  Table opponent_turn_table_after_blue_capture(0, opponent_after_blue_capture_database_path.c_str(), 2, configuration_count_opponent_after_capture);
  Table opponent_turn_table_after_red_capture(0, opponent_after_red_capture_database_path.c_str(), 2, configuration_count_opponent_after_capture);
  Table self_turn_table_reversed_after_blue_capture(0, self_reversed_after_blue_capture_database_path.c_str(), 2, configuration_count_self_reversed_after_blue_capture);
  Table self_turn_table_reversed_after_red_capture(0, self_reversed_after_red_capture_database_path.c_str(), 2, configuration_count_self_reversed_after_red_capture);
  Table opponent_turn_table_reversed_after_blue_capture(0, opponent_reversed_after_blue_capture_database_path.c_str(), 2, configuration_count_opponent_reversed_after_capture);
  Table opponent_turn_table_reversed_after_red_capture(0, opponent_reversed_after_red_capture_database_path.c_str(), 2, configuration_count_opponent_reversed_after_capture);
    
  while(true) {
    if(self_blue_count == opponent_blue_count && self_red_count == opponent_red_count) {
      cout << "[analysis] iteration: " << iteration << ", pieces (self blue, self red, opponent blue, opponent red): ("
	   << self_blue_count << ", " << self_red_count << ", " << opponent_blue_count << ", " << opponent_red_count << ")" << endl;
      
      // Reset the shutdown flag before starting this pass.
      shutdown_requested = false;
      
      if(iteration % 2 == 1) {
	thread coordinator_thread(run_coordinator, iteration, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count,
				  ref(self_turn_table), self_database_path.c_str(),
				  cref(opponent_turn_table), cref(opponent_turn_table_after_blue_capture), cref(opponent_turn_table_after_red_capture),
				  cref(opponent_turn_table));
	
	thread worker_threads[worker_count];
	for(int worker_index = 0; worker_index < worker_count; worker_index++){
	  worker_threads[worker_index] = thread(run_worker, iteration, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count,
						cref(*zdd), cref(*zdd_opponent_after_capture), cref(*zdd_opponent_after_capture),
						cref(*zdd));
	}
	
	// Wait for the coordinator and all workers to finish this pass.
	coordinator_thread.join(); 
	for(int worker_index = 0; worker_index < worker_count; worker_index++){
	  worker_threads[worker_index].join();
	}
      } else {
	thread coordinator_thread(run_coordinator, iteration, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count,
				  ref(opponent_turn_table), opponent_database_path.c_str(),
				  cref(self_turn_table), cref(self_turn_table_after_blue_capture), cref(self_turn_table_after_red_capture),
				  cref(self_turn_table));
	
	thread worker_threads[worker_count];
	for(int worker_index = 0; worker_index < worker_count; worker_index++) {
	  worker_threads[worker_index] = thread(run_worker, iteration, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count,
						cref(*zdd), cref(*zdd_self_after_blue_capture), cref(*zdd_self_after_red_capture),
						cref(*zdd));
	}
	
	// Wait for the coordinator and all workers to finish this pass.
	coordinator_thread.join(); 
	for(int worker_index = 0; worker_index < worker_count; worker_index++){
	  worker_threads[worker_index].join();
	}
      }
      
      if(label_update_count == 0) {
	cout << "[analysis] Finished: no label updates" << endl;
	break;
      }
    } else {
      assert(self_blue_count != opponent_blue_count || self_red_count != opponent_red_count);

      if(!start_with_reversed_orientation) {
	// First, process (self_blue_count, self_red_count, opponent_blue_count, opponent_red_count)
	cout << "[analysis] iteration: " << iteration << ", pieces (self blue, self red, opponent blue, opponent red): ("
	     << self_blue_count << ", " << self_red_count << ", " << opponent_blue_count << ", " << opponent_red_count << ")" << endl;
	
	// Reset the shutdown flag before starting this pass.
	shutdown_requested = false;
	
	if(iteration % 2 == 1) {
	  thread coordinator_thread(run_coordinator, iteration, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count,
				    ref(self_turn_table), self_database_path.c_str(),
				    cref(opponent_turn_table), cref(opponent_turn_table_after_blue_capture), cref(opponent_turn_table_after_red_capture),
				    cref(opponent_turn_table_reversed));
	  
	  thread worker_threads[worker_count];
	  for(int worker_index = 0; worker_index < worker_count; worker_index++){
	    worker_threads[worker_index] = thread(run_worker, iteration, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count,
						  cref(*zdd), cref(*zdd_opponent_after_capture), cref(*zdd_opponent_after_capture),
						  cref(*zdd_reversed));
	  }
	  
	  // Wait for the coordinator and all workers to finish this pass.
	  coordinator_thread.join(); 
	  for(int worker_index = 0; worker_index < worker_count; worker_index++){
	    worker_threads[worker_index].join();
	  }
	} else {
	  thread coordinator_thread(run_coordinator, iteration, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count,
				    ref(opponent_turn_table), opponent_database_path.c_str(),
				    cref(self_turn_table), cref(self_turn_table_after_blue_capture), cref(self_turn_table_after_red_capture),
				    cref(self_turn_table_reversed));
	  
	  thread worker_threads[worker_count];
	  for(int worker_index = 0; worker_index < worker_count; worker_index++) {
	    worker_threads[worker_index] = thread(run_worker, iteration, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count,
						  cref(*zdd), cref(*zdd_self_after_blue_capture), cref(*zdd_self_after_red_capture),
						  cref(*zdd_reversed));
	  }
	
	  // Wait for the coordinator and all workers to finish this pass.
	  coordinator_thread.join(); 
	  for(int worker_index = 0; worker_index < worker_count; worker_index++){
	    worker_threads[worker_index].join();
	  }
	}
      }

      // Next, process (opponent_blue_count, opponent_red_count, self_blue_count, self_red_count)
      cout << "[analysis] iteration: " << iteration << ", pieces (self blue, self red, opponent blue, opponent red): ("
	   << opponent_blue_count << ", " << opponent_red_count << ", " << self_blue_count << ", " << self_red_count << ")" << endl;
      int forward_label_update_count = label_update_count;
      
      // Reset the shutdown flag before starting this pass.
      shutdown_requested = false;
      
      if(iteration % 2 == 1) {
	thread coordinator_thread(run_coordinator, iteration, opponent_blue_count, opponent_red_count, self_blue_count, self_red_count,
				  ref(self_turn_table_reversed), self_reversed_database_path.c_str(),
				  cref(opponent_turn_table_reversed), cref(opponent_turn_table_reversed_after_blue_capture), cref(opponent_turn_table_reversed_after_red_capture),
				  cref(opponent_turn_table));
	
	thread worker_threads[worker_count];
	for(int worker_index = 0; worker_index < worker_count; worker_index++){
	  worker_threads[worker_index] = thread(run_worker, iteration, opponent_blue_count, opponent_red_count, self_blue_count, self_red_count,
						cref(*zdd_reversed), cref(*zdd_opponent_reversed_after_capture), cref(*zdd_opponent_reversed_after_capture),
						cref(*zdd));
	}
	
	// Wait for the coordinator and all workers to finish this pass.
	coordinator_thread.join(); 
	for(int worker_index = 0; worker_index < worker_count; worker_index++){
	  worker_threads[worker_index].join();
	}
      } else {
	thread coordinator_thread(run_coordinator, iteration, opponent_blue_count, opponent_red_count, self_blue_count, self_red_count,
				  ref(opponent_turn_table_reversed), opponent_reversed_database_path.c_str(),
				  cref(self_turn_table_reversed), cref(self_turn_table_reversed_after_blue_capture), cref(self_turn_table_reversed_after_red_capture),
				  cref(self_turn_table));
	
	thread worker_threads[worker_count];
	for(int worker_index = 0; worker_index < worker_count; worker_index++) {
	  worker_threads[worker_index] = thread(run_worker, iteration, opponent_blue_count, opponent_red_count, self_blue_count, self_red_count,
						cref(*zdd_reversed), cref(*zdd_self_reversed_after_blue_capture), cref(*zdd_self_reversed_after_red_capture),
						cref(*zdd));
	}
	
	// Wait for the coordinator and all workers to finish this pass.
	coordinator_thread.join(); 
	for(int worker_index = 0; worker_index < worker_count; worker_index++){
	  worker_threads[worker_index].join();
	}
      }
      
      if(label_update_count == 0 && forward_label_update_count == 0) {
	cout << "[analysis] Finished: no label updates" << endl;
	break;
      }
      start_with_reversed_orientation = false;
    }
    iteration++;
  }
  return 0;
}

// Build: make bin/main
// Example: ./bin/main 1 1 1 1 1 forward
// Batch execution: ./scripts/run_main.sh
