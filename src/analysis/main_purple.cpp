// Construct perfect-information Purple-piece Geister endgame databases.
// Outcomes: win, loss, and unresolved; each entry uses 2 bits.
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

constexpr int worker_count = 7;   // number of worker threads
constexpr int input_queue_capacity = 2048;    
constexpr int output_queue_capacity = 256;
constexpr int max_legal_moves = 32;
constexpr int max_compatible_configurations = 70;   // unused in the purple-piece version

condition_variable coordinator_cv;   // signals completed worker tasks
condition_variable worker_cv;   // signals tasks or shutdown
mutex queue_mutex;   // protects task queues and the shutdown flag

bool shutdown_requested;   // worker shutdown request

// Reusable task containing a configuration ID and one result per legal move.
class AnalysisTask {
private:
  unsigned long long int m_configuration_id;   // configuration ID
  int m_legal_move_count;   // number of legal moves
  
  // Capture codes: 0 = none, 1 = blue, 2 = red.
  int m_captured_piece_type[max_legal_moves];
  // Nonnegative results are child IDs; -1 = terminal win, -2 = terminal loss.
  long long int m_child_results[max_legal_moves];

public:
  AnalysisTask() noexcept {}
  AnalysisTask(unsigned long long int configuration_id) noexcept : m_configuration_id(configuration_id) {}
  void set_configuration(unsigned long long int configuration_id) noexcept { m_configuration_id = configuration_id; }
  void set_move_children(int legal_move_count, int captured_piece_type[max_legal_moves], long long int child_results[max_legal_moves]) noexcept {
    assert(legal_move_count > 0 && legal_move_count <= max_legal_moves);
    m_legal_move_count = legal_move_count;
    for(int i = 0; i < m_legal_move_count; i++) {
      m_captured_piece_type[i] = captured_piece_type[i];
      m_child_results[i] = child_results[i];
    }
  }
  unsigned long long int get_configuration_id() const noexcept { return m_configuration_id; }
  int get_move_children(int captured_piece_type[max_legal_moves], long long int child_results[max_legal_moves]) const noexcept {
    assert(m_legal_move_count > 0 && m_legal_move_count <= max_legal_moves);
    for(int i = 0; i < m_legal_move_count; i++) {
      captured_piece_type[i] = m_captured_piece_type[i];
      child_results[i] = m_child_results[i];
    }
    return m_legal_move_count;
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
	  
unsigned long long int win_assignment_count = 0, loss_assignment_count = 0;

unsigned long long int label_update_count = 0ULL;

deque<AnalysisTask *> input_queue;
deque<AnalysisTask *> output_queue;

// Schedule tasks, assign outcomes, and write the database.
static void run_coordinator(int iteration, int self_blue_count, int self_red_count, int opponent_blue_count, int opponent_red_count,
		 Table& parent_table, const char* output_database_path,
		 const Table& child_table, const Table& child_table_after_blue_capture, const Table& child_table_after_red_capture) noexcept {
  std::cout << "[analysis] Starting configuration scan" << endl;

  win_assignment_count = 0, loss_assignment_count = 0;
  label_update_count = 0ULL;
  
  unsigned long long int configuration_count = placement_count[self_blue_count][self_red_count][opponent_blue_count + opponent_red_count];
  unsigned long long int scanned_configuration_count = 0ULL;
  unsigned long long int completed_configuration_count = 0ULL;
  
  int available_task_count = input_queue_capacity + output_queue_capacity + worker_count;
  AnalysisTask* available_tasks[available_task_count];
  for(int i = 0; i < available_task_count; i++) available_tasks[i] = new AnalysisTask;
  
  while(true) {
    unique_lock<mutex> queue_lock(queue_mutex);
    // Wait for queue capacity or results; release the mutex while blocked.
    coordinator_cv.wait(queue_lock, [&](){
      return (((input_queue.size() < input_queue_capacity) && (output_queue.size() < output_queue_capacity) && (scanned_configuration_count < configuration_count))
	      || (0 < output_queue.size())); });
    
    if((input_queue.size() < input_queue_capacity) && (output_queue.size() < output_queue_capacity) && scanned_configuration_count < configuration_count) {

      // Skip configurations whose outcome is already known.
      unsigned int initial_label;

      while(scanned_configuration_count < configuration_count && (initial_label = parent_table.get(scanned_configuration_count)) != v_unknown) {
	
	scanned_configuration_count++;
	completed_configuration_count++;
      }
      if(scanned_configuration_count >= configuration_count) {
	queue_lock.unlock();
	continue;
      }
                
      assert(available_task_count >= 1);
      AnalysisTask *task = available_tasks[ --available_task_count ];
      task->set_configuration(scanned_configuration_count);
      input_queue.push_front(task);
      queue_lock.unlock();
      scanned_configuration_count++;
      
      if(scanned_configuration_count % 1000000000ULL == 0ULL) cout << "[progress] scanned_configurations: " << scanned_configuration_count << ", win_assignments: " << win_assignment_count  << ", loss_assignments: " << loss_assignment_count << endl;
      
      worker_cv.notify_one();
    } else {
      if (0 < output_queue.size()) {
	assert(0 < output_queue.size());
	deque<AnalysisTask *> completed_tasks;
	// Consume results outside the lock; only the coordinator assigns outcomes.
	swap(completed_tasks, output_queue);
	
	output_queue.clear();
	queue_lock.unlock();
	completed_configuration_count += completed_tasks.size();
	for(unsigned int task_index = 0; task_index < completed_tasks.size(); task_index++) {
	  unsigned long long int configuration_id = completed_tasks[task_index]->get_configuration_id();
	  
	  int captured_piece_type[max_legal_moves];
	  long long int child_results[max_legal_moves];
	  int legal_move_count = completed_tasks[task_index]->get_move_children(captured_piece_type, child_results);
	    
	  assert(legal_move_count > 0);
	  int winning_move_count = 0;   // children yielding a win
	  int losing_move_count = 0;   // children yielding a loss
	  bool label_assigned = false;
	  for(int j = 0; j < legal_move_count; j++) {
	    long long int child_result = child_results[j];
	    if(child_result >= 0) {
	      int child_label;
	      int captured_piece = captured_piece_type[j];
	      
	      if(captured_piece == 0) child_label = child_table.get((unsigned long long int)child_result);
	      
	      else if(captured_piece == 1) child_label = child_table_after_blue_capture.get((unsigned long long int)child_result);
	      
	      else {
		assert(captured_piece == 2);
		child_label = child_table_after_red_capture.get((unsigned long long int)child_result);
	      }
	      if(child_label == v_win) winning_move_count++;
	      else if(child_label == v_loss) losing_move_count++;
	      else {
		assert(child_label == v_unknown);
	      }
	    } else {
	      if(child_result == -1) {
		
		winning_move_count++;
	      } else if(child_result == -2) {
		
		losing_move_count++;
	      } else {
		
	      }
	    }

	    if(iteration % 2 == 1) {
	      if(winning_move_count >= 1) {
		parent_table.set(configuration_id, v_win);
		win_assignment_count++;
		label_update_count++;
		label_assigned = true;
		break;
	      }
	    } else {
	      if(losing_move_count >= 1) {
		parent_table.set(configuration_id, v_loss);
		loss_assignment_count++;
		label_update_count++;
		label_assigned = true;
		break;
	      }
	    }
	  }

	  
	  if(!label_assigned) {
	    if(iteration % 2 == 1) {
	      if(losing_move_count == legal_move_count) {
		parent_table.set(configuration_id, v_loss);
		loss_assignment_count++;
		label_update_count++;
	      }
	    } else {
	      if(winning_move_count == legal_move_count) {
		parent_table.set(configuration_id, v_win);
		win_assignment_count++;
		label_update_count++;
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
      
      if(completed_configuration_count >= configuration_count) break;
    }
  }
  
  cout << "[output] Writing database: " << output_database_path << endl;
  {
    TableWriter table_writer(iteration, output_database_path, configuration_count, 2);
    for(unsigned long long int i = 0; i < configuration_count; i++) {
      table_writer.write(parent_table.get(i));
    }
    table_writer.flush();
    table_writer.print_statistics();
  }
  
  {
    unique_lock<mutex> queue_lock(queue_mutex);
    shutdown_requested = true;
  }
  worker_cv.notify_all();
}

// Reconstruct queued positions and generate child IDs or terminal outcomes.
static void run_worker(int iteration, int self_blue_count, int self_red_count, int opponent_blue_count, int opponent_red_count,
		   const ZDD& zdd_parent, const ZDD& zdd_child_after_blue_capture, const ZDD& zdd_child_after_red_capture) noexcept {
  AnalysisTask *task;
  while (true) {
    unique_lock<mutex> queue_lock(queue_mutex);
    
    // Wait for a task or shutdown; reacquire the mutex before returning.
    worker_cv.wait(queue_lock, [&](){ if(0 < input_queue.size()) return true;
	return shutdown_requested; });
    
    if(shutdown_requested) break;
    assert(0 < input_queue.size());
    task = input_queue.back();
    input_queue.pop_back();
    queue_lock.unlock();
    
    unsigned long long int configuration_id = task->get_configuration_id();
    Position position;
    position.load_configuration(configuration_id, zdd_parent, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count);
    Action actions[max_legal_moves];
    int legal_move_count = position.generate_legal_actions(actions, iteration);
    assert(legal_move_count > 0 && legal_move_count < max_legal_moves);
      
    int captured_piece_type[max_legal_moves] = {};
    long long int child_results[max_legal_moves];
      
    for(int i = 0; i < legal_move_count; i++) {
      int move_result = position.apply_action(actions[i]);
      if(move_result >= 0) {
	if(move_result == 0) {
	  child_results[i] = position.get_zdd_id(zdd_parent);
	} else if(move_result == 1) {
	  child_results[i] = position.get_zdd_id(zdd_child_after_blue_capture);
	  captured_piece_type[i] = 1;
	} else {
	  assert(move_result == 2);
	  child_results[i] = position.get_zdd_id(zdd_child_after_red_capture);
	  captured_piece_type[i] = 2;
	}
      } else {
	child_results[i] = move_result;
      }
      position.undo_action();
    }
    task->set_move_children(legal_move_count, captured_piece_type, child_results);
         
    queue_lock.lock();
    output_queue.push_front(task);
    queue_lock.unlock();
    coordinator_cv.notify_one();
  }
}

int main(int argc, char *argv[]) {
  if(argc == 2 && string(argv[1]) == "--help") {
    cout << "Usage: " << argv[0]
	 << " ITERATION SELF_BLUE SELF_RED OPPONENT_BLUE OPPONENT_RED" << endl;
    cout << "The first processed pass is ITERATION + 1; no orientation argument is required." << endl;
    return 0;
  }
  if(argc != 6) {
    cerr << "Expected five arguments. Use --help for usage." << endl;
    return 1;
  }
  
  int iteration = atoi(argv[1]);
  int self_blue_count = atoi(argv[2]), self_red_count = atoi(argv[3]), opponent_blue_count = atoi(argv[4]), opponent_red_count = atoi(argv[5]);

  string self_database_path = "data/db_purple/" + table_filename_prefixes[0] + '_'
    + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  string opponent_database_path = "data/db_purple/" + table_filename_prefixes[1] + '_'
    + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  string opponent_after_blue_capture_database_path = "data/db_purple/" + table_filename_prefixes[1] + '_'
    + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-' + to_string(opponent_blue_count - 1) + '-' + to_string(opponent_red_count) + ".bin";
  string opponent_after_red_capture_database_path = "data/db_purple/" + table_filename_prefixes[1] + '_'
    + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count - 1) + ".bin";
  string self_after_blue_capture_database_path = "data/db_purple/" + table_filename_prefixes[0] + '_'
    + to_string(self_blue_count - 1) + '-' + to_string(self_red_count) + '-' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  string self_after_red_capture_database_path = "data/db_purple/" + table_filename_prefixes[0] + '_'
    + to_string(self_blue_count) + '-' + to_string(self_red_count - 1) + '-' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  
  unsigned long long int configuration_count = placement_count[self_blue_count][self_red_count][opponent_blue_count + opponent_red_count];
  unsigned long long int configuration_count_self_after_blue_capture = placement_count[self_blue_count - 1][self_red_count][opponent_blue_count + opponent_red_count];
  unsigned long long int configuration_count_self_after_red_capture = placement_count[self_blue_count][self_red_count - 1][opponent_blue_count + opponent_red_count];
  unsigned long long int configuration_count_opponent_after_capture = placement_count[self_blue_count][self_red_count][opponent_blue_count + opponent_red_count - 1];
  
  unique_ptr<ZDD> zdd = make_unique<ZDD>(self_blue_count, self_red_count, opponent_blue_count + opponent_red_count);
  unique_ptr<ZDD> zdd_opponent_after_capture = make_unique<ZDD>(self_blue_count, self_red_count, opponent_blue_count + opponent_red_count - 1);
  unique_ptr<ZDD> zdd_self_after_blue_capture = make_unique<ZDD>(self_blue_count - 1, self_red_count, opponent_blue_count + opponent_red_count);
  unique_ptr<ZDD> zdd_self_after_red_capture = make_unique<ZDD>(self_blue_count, self_red_count - 1, opponent_blue_count + opponent_red_count);

  int self_turn_iteration, opponent_turn_iteration;
  if(iteration % 2 == 1) {
    self_turn_iteration = iteration - 2;
    opponent_turn_iteration = iteration - 1;
  } else {
    assert(iteration % 2 == 0);
    self_turn_iteration = iteration - 1;
    opponent_turn_iteration = iteration - 2;
  }
  
  Table self_turn_table(self_turn_iteration, self_database_path.c_str(), 2, configuration_count);
  Table opponent_turn_table(opponent_turn_iteration, opponent_database_path.c_str(), 2, configuration_count);
  Table self_turn_table_after_blue_capture(0, self_after_blue_capture_database_path.c_str(), 2, configuration_count_self_after_blue_capture);
  Table self_turn_table_after_red_capture(0, self_after_red_capture_database_path.c_str(), 2, configuration_count_self_after_red_capture);
  Table opponent_turn_table_after_blue_capture(0, opponent_after_blue_capture_database_path.c_str(), 2, configuration_count_opponent_after_capture);
  Table opponent_turn_table_after_red_capture(0, opponent_after_red_capture_database_path.c_str(), 2, configuration_count_opponent_after_capture);
    
  while(true) {
    // The first processed pass is the iteration after the supplied value.
    iteration++;
    cout << "[analysis] iteration: " << iteration << ", pieces (self blue, self red, opponent blue, opponent red): ("
	 << self_blue_count << ", " << self_red_count << ", " << opponent_blue_count << ", " << opponent_red_count << ")" << endl;
      
    // Reset the shutdown flag before launching this pass.
    shutdown_requested = false;
    
    if(iteration % 2 == 1) {
      thread coordinator_thread(run_coordinator, iteration, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count,
				ref(self_turn_table), self_database_path.c_str(),
				cref(opponent_turn_table), cref(opponent_turn_table_after_blue_capture), cref(opponent_turn_table_after_red_capture));
      
      thread worker_threads[worker_count];
      for(int worker_index = 0; worker_index < worker_count; worker_index++){
	worker_threads[worker_index] = thread(run_worker, iteration, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count,
					      cref(*zdd), cref(*zdd_opponent_after_capture), cref(*zdd_opponent_after_capture));
      }
	
      // Wait for the coordinator and all workers to finish.
      coordinator_thread.join(); 
      for(int worker_index = 0; worker_index < worker_count; worker_index++){
	worker_threads[worker_index].join();
      }
    } else {
      thread coordinator_thread(run_coordinator, iteration, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count,
		     ref(opponent_turn_table), opponent_database_path.c_str(),
		     cref(self_turn_table), cref(self_turn_table_after_blue_capture), cref(self_turn_table_after_red_capture));
      
      thread worker_threads[worker_count];
      for(int worker_index = 0; worker_index < worker_count; worker_index++) {
	worker_threads[worker_index] = thread(run_worker, iteration, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count,
				     cref(*zdd), cref(*zdd_self_after_blue_capture), cref(*zdd_self_after_red_capture));
      }
      
      // Wait for the coordinator and all workers to finish.
      coordinator_thread.join(); 
      for(int worker_index = 0; worker_index < worker_count; worker_index++){
	worker_threads[worker_index].join();
      }
    }

    // Preserve the original stopping rule: end after one pass with no updates.
    if(label_update_count == 0) {
      cout << "[analysis] Finished: no label updates in this pass" << endl;
      break;
    }
  }

  return 0;
}


// Build: make bin/main_purple (defines USE_PURPLE).
// Example: ./bin/main_purple 1 1 1 1 1
// Batch execution: ./scripts/run_main_purple.sh
