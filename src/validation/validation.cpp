// Verify ZDD enumeration/round trips and stored-label successor conditions.
#include <memory>
#include <iostream>
#include <vector>
#include <exception>
#include <cassert>
#include "../common/node.hpp"
#include "../common/table.hpp"
#include "../common/zdd_geister.hpp"
#include "../common/posi_geister.hpp"

using std::unique_ptr;
using std::make_unique;
using std::vector;

constexpr int max_enumeration_depth = 144;

const string table_filename_prefixes[2] = {"self_table", "enemy_table"};

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

// Traverse branches in ID order and compare each enumerated board with its ZDD mapping.
void verify_zdd_enumeration(Node& current_node, vector<unsigned char>& board_object_ids, const ZDD& zdd, int depth, unsigned long long int& enumerated_configuration_id, int self_blue_count, int self_red_count, int opponent_blue_count, int opponent_red_count) {
  if(depth == max_enumeration_depth) {
    assert(board_object_ids.size() == 36);
    unsigned char decoded_board_object_ids[36];

    // Compare the enumerated board with the board decoded from the same ID.
    zdd.compute_array(enumerated_configuration_id, decoded_board_object_ids, 36);
    
    for(size_t i = 0; i < board_object_ids.size(); i++) {
      if(board_object_ids[i] != decoded_board_object_ids[i]) {
	cerr << "[error] ZDD decoding mismatch, configuration_id: " << enumerated_configuration_id << ", square_index: " << i << endl;
	cerr << "enumerated_object_id: " << +board_object_ids[i] << ", decoded_object_id: " << +decoded_board_object_ids[i] << endl;
	terminate();
      }
    }

    // Encoding the enumerated board must recover its enumeration ID.
    unsigned long long int encoded_configuration_id = zdd.compute_id(board_object_ids.data(), board_object_ids.size());
    if(enumerated_configuration_id != encoded_configuration_id) {
      cerr << "[error] ZDD encoding mismatch, configuration_id: " << enumerated_configuration_id << endl;
      terminate();
    }

    if(enumerated_configuration_id % 100000000 == 0) {
      cout << "[progress] ZDD round trip passed, configuration_id: " << enumerated_configuration_id << ": ";
      for(char x : board_object_ids) {
	cout << +x;
      }
      cout << endl;
    }
    
    enumerated_configuration_id++;
    return;
  }

  for(int x = 0; x < 2; x++) {
    // Skip branches leading to the zero terminal.
    if(current_node.branch_leads_to_zero_terminal(depth, x, self_blue_count, self_red_count, opponent_blue_count + opponent_red_count)) {
      continue;
    } else {
      unique_ptr<Node> child_node = make_unique<Node>(current_node, x);

      if(x == 0) {
	verify_zdd_enumeration(*child_node, board_object_ids, zdd, depth + 1, enumerated_configuration_id, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count);
      } else {
	board_object_ids.push_back(current_node.get_object_state_id());
	verify_zdd_enumeration(*child_node, board_object_ids, zdd, depth + 1, enumerated_configuration_id, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count);
	board_object_ids.pop_back();
      }
    }
  }
  return;
}

// Return true when an assigned FW or CL label violates its successor condition.
// child_labels is indexed by [move][compatible third-party configuration].
bool has_invalid_win_or_can_lose_label(int parent_label, int child_labels[32][70], int legal_move_count, int compatible_configuration_count,
				      int play_to_move, int self_blue_count, int self_red_count, int opponent_blue_count, int opponent_red_count) {
  assert(parent_label == v_win || parent_label == v_can_lose);

  assert(compatible_configuration_count > 0 && compatible_configuration_count <= 70);
  assert(legal_move_count > 0 && legal_move_count < 32);
  
  if(parent_label == v_win) {
    if(play_to_move == 1) {
      for(int j = 0; j < legal_move_count; j++) {
	int winning_configuration_count = 0;
	for(int m = 0; m < compatible_configuration_count; m++) {
	  if(child_labels[j][m] == v_win) winning_configuration_count++;
	}
	if(winning_configuration_count == compatible_configuration_count) return false;
      }
      return true;
    } else {
      for(int j = 0; j < legal_move_count; j++) {
	for(int m = 0; m < compatible_configuration_count; m++) {
	  if(child_labels[j][m] != v_win) return true;
	}
      }
      return false;
    }
  } else {
    assert(parent_label == v_can_lose);
    if(play_to_move == 1) {
      for(int j = 0; j < legal_move_count; j++) {
	int can_lose_configuration_count = 0;
	for(int m = 0; m < compatible_configuration_count; m++) {
	  if(child_labels[j][m] == v_can_lose || child_labels[j][m] == v_lose) can_lose_configuration_count++;
	}
	if(can_lose_configuration_count < 1) return true;
      }
      return false;
    } else {
      for(int j = 0; j < legal_move_count; j++) {
	for(int m = 0; m < compatible_configuration_count; m++) {
	  if(child_labels[j][m] == v_can_lose || child_labels[j][m] == v_lose) {
	    return false;
	  }
	}
      }
      return true;
    }
  }
}

// Return true when an assigned FL label violates its successor or reversed-view condition.
bool has_invalid_loss_label(int parent_label, int child_labels[32][70], int reversed_view_labels[70], int legal_move_count, int compatible_configuration_count,
					int play_to_move, int self_blue_count, int self_red_count, int opponent_blue_count, int opponent_red_count) {
  assert(parent_label == v_lose);

  assert(compatible_configuration_count > 0 && compatible_configuration_count <= 70);
  assert(legal_move_count > 0 && legal_move_count < 32);
  
  if(play_to_move == 1) {
    for(int j = 0; j < legal_move_count; j++) {
      for(int m = 0; m < compatible_configuration_count; m++) {
	if(child_labels[j][m] != v_lose) return true;
      }
    }
    return false;
  } else {
    assert(play_to_move == 2);
    int winning_reversed_view_count = 0;
    for(int m = 0; m < compatible_configuration_count; m++) {
      if(reversed_view_labels[m] == v_win) winning_reversed_view_count++;
    }
    if(winning_reversed_view_count == compatible_configuration_count) return false;
    else return true;
  }
}

int main(int argc, char *argv[]) {
  if(argc == 2 && string(argv[1]) == "--help") {
    cout << "Usage: " << argv[0]
	 << " SELF_BLUE SELF_RED OPPONENT_BLUE OPPONENT_RED" << endl;
    cout << "Verify ZDD mappings and assigned labels for both players to move." << endl;
    return 0;
  }
  if(argc != 5) {
    cerr << "[error] Expected four arguments. Use --help for usage." << endl;
    return 1;
  }
  
  int self_blue_count = atoi(argv[1]), self_red_count = atoi(argv[2]), opponent_blue_count = atoi(argv[3]), opponent_red_count = atoi(argv[4]);
  unsigned long long int configuration_id = 0;
  string self_database_path = "data/db/" + table_filename_prefixes[0] + '_'
    + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  string opponent_database_path = "data/db/" + table_filename_prefixes[1] + '_'
    + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  string self_reversed_database_path = "data/db/" + table_filename_prefixes[0] + '_'
    + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + '-' + to_string(self_blue_count) + '-' + to_string(self_red_count) + ".bin";
  string opponent_reversed_database_path = "data/db/" + table_filename_prefixes[1] + '_'
    + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + '-' + to_string(self_blue_count) + '-' + to_string(self_red_count) + ".bin";
  string opponent_after_blue_capture_database_path = "data/db/" + table_filename_prefixes[1] + '_'
    + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-' + to_string(opponent_blue_count - 1) + '-' + to_string(opponent_red_count) + ".bin";
  string opponent_after_red_capture_database_path = "data/db/" + table_filename_prefixes[1] + '_'
    + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count - 1) + ".bin";
  string self_after_blue_capture_database_path = "data/db/" + table_filename_prefixes[0] + '_'
    + to_string(self_blue_count - 1) + '-' + to_string(self_red_count) + '-' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  string self_after_red_capture_database_path = "data/db/" + table_filename_prefixes[0] + '_'
    + to_string(self_blue_count) + '-' + to_string(self_red_count - 1) + '-' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  
  unsigned long long int configuration_count = placement_count[self_blue_count][self_red_count][opponent_blue_count + opponent_red_count];
  unsigned long long int configuration_count_reversed = placement_count[opponent_blue_count][opponent_red_count][self_blue_count + self_red_count];
  unsigned long long int configuration_count_self_after_blue_capture = placement_count[self_blue_count - 1][self_red_count][opponent_blue_count + opponent_red_count];
  unsigned long long int configuration_count_self_after_red_capture = placement_count[self_blue_count][self_red_count - 1][opponent_blue_count + opponent_red_count];
  unsigned long long int configuration_count_opponent_after_capture = placement_count[self_blue_count][self_red_count][opponent_blue_count + opponent_red_count - 1];

  unique_ptr<ZDD> zdd = make_unique<ZDD>(self_blue_count, self_red_count, opponent_blue_count + opponent_red_count);
  unique_ptr<ZDD> zdd_reversed = make_unique<ZDD>(opponent_blue_count, opponent_red_count, self_blue_count + self_red_count);
  unique_ptr<ZDD> zdd_opponent_after_capture = make_unique<ZDD>(self_blue_count, self_red_count, opponent_blue_count + opponent_red_count - 1);
  unique_ptr<ZDD> zdd_self_after_blue_capture = make_unique<ZDD>(self_blue_count - 1, self_red_count, opponent_blue_count + opponent_red_count);
  unique_ptr<ZDD> zdd_self_after_red_capture = make_unique<ZDD>(self_blue_count, self_red_count - 1, opponent_blue_count + opponent_red_count);
  
  {
    unique_ptr<Node> root = make_unique<Node>();
    vector<unsigned char> board_object_ids;
  
    verify_zdd_enumeration(*root, board_object_ids, *zdd, 0, configuration_id, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count);
  }

  cout << "[validation] enumerated_configurations: " << configuration_id << endl;

  cout << "[validation] ZDD checks passed, pieces (self blue, self red, opponent blue, opponent red): (" << self_blue_count << "," << self_red_count << "," << opponent_blue_count << "," << opponent_red_count << ")" << endl;

  struct DatabaseRequirement {
    string path;
    bool required;
  };

  // Capture databases are needed only when the captured color still has pieces remaining.
  const DatabaseRequirement database_requirements[] = {
					    {self_database_path, true},
					    {opponent_database_path, true},
					    {self_reversed_database_path, true},
					    {opponent_reversed_database_path, true},
					    {self_after_blue_capture_database_path, self_blue_count > 1},
					    {self_after_red_capture_database_path, self_red_count > 1},
					    {opponent_after_blue_capture_database_path, opponent_blue_count > 1},
					    {opponent_after_red_capture_database_path, opponent_red_count > 1}
  };

  for(const DatabaseRequirement& database : database_requirements) {
    if(!database.required) continue;
    
    std::ifstream database_file(database.path, std::ios::binary);
    if(!database_file) {
      cerr << "[error] Failed to open required database: "
	   << database.path << endl;
      return 1;
    }
  }

  // Read databases containing 2-bit label entries.
  Table self_turn_table(0, self_database_path.c_str(), 2, configuration_count);
  Table opponent_turn_table(0, opponent_database_path.c_str(), 2, configuration_count);
  Table self_turn_table_reversed(0, self_reversed_database_path.c_str(), 2, configuration_count_reversed);
  Table opponent_turn_table_reversed(0, opponent_reversed_database_path.c_str(), 2, configuration_count_reversed);
  Table self_turn_table_after_blue_capture(0, self_after_blue_capture_database_path.c_str(), 2, configuration_count_self_after_blue_capture);
  Table self_turn_table_after_red_capture(0, self_after_red_capture_database_path.c_str(), 2, configuration_count_self_after_red_capture);
  Table opponent_turn_table_after_blue_capture(0, opponent_after_blue_capture_database_path.c_str(), 2, configuration_count_opponent_after_capture);
  Table opponent_turn_table_after_red_capture(0, opponent_after_red_capture_database_path.c_str(), 2, configuration_count_opponent_after_capture);
  
  Posi position;
  bool validation_failed = false;

  // Verify both turn databases using forward-generated successors.
  for(int player_to_move = 1; player_to_move <= 2; player_to_move++) {
    for(unsigned long long int i = 0ULL; i < configuration_count; i++) {
      position.make_posi(i, *zdd, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count);
      Action actions[32] = {};
      unsigned char compatible_boards[70][36] = {};
      int legal_move_count = position.compute_actions(actions, player_to_move);
      int compatible_configuration_count = position.getunknowninfo(compatible_boards);
    
      int parent_label;
      if(player_to_move == 1) parent_label = self_turn_table.get(i);
      else parent_label = opponent_turn_table.get(i);
      
      assert(parent_label >= 0 && parent_label <= 5);

      assert(legal_move_count > 0);
      int child_labels[32][70] = {};
      int reversed_view_labels[70] = {};
      for(int j = 0; j < legal_move_count; j++) {
	for(int m = 0; m < compatible_configuration_count; m++) {
	  position.make_posi_n(compatible_boards, m);
	  int move_result = position.make_action(actions[j]);
	  if(move_result >= 0) {
	    position.make_posi_myself();
	    if(player_to_move == 1) {
	      if(move_result == 0) child_labels[j][m] = opponent_turn_table.get(position.getzddnum(*zdd));
	      else if(move_result == 1) child_labels[j][m] = opponent_turn_table_after_blue_capture.get(position.getzddnum(*zdd_opponent_after_capture));
	      else {
		assert(move_result == 2);
		child_labels[j][m] = opponent_turn_table_after_red_capture.get(position.getzddnum(*zdd_opponent_after_capture));
	      }
	    } else {
	      if(move_result == 0) child_labels[j][m] = self_turn_table.get(position.getzddnum(*zdd));
	      else if(move_result == 1) child_labels[j][m] = self_turn_table_after_blue_capture.get(position.getzddnum(*zdd_self_after_blue_capture));
	      else {
		assert(move_result == 2);
		child_labels[j][m] = self_turn_table_after_red_capture.get(position.getzddnum(*zdd_self_after_red_capture));
	      }
	    }
	  } else {
	    if(move_result == -1) child_labels[j][m] = v_win;
	    else {
	      assert(move_result == -2);
	      child_labels[j][m] = v_lose;
	    }
	  }
	  position.undo_action();
	}
      }
      // Convert compatible boards to the opponent viewpoint for FL checks.
      for(int m = 0; m < compatible_configuration_count; m++) {
	position.make_posi_n(compatible_boards, m);
	position.make_posi_opponent();
	if(player_to_move == 1) reversed_view_labels[m] = opponent_turn_table_reversed.get(position.getzddnum(*zdd_reversed));
	else reversed_view_labels[m] = self_turn_table_reversed.get(position.getzddnum(*zdd_reversed));
      }
      if(parent_label == v_win || parent_label == v_can_lose) {
	if(has_invalid_win_or_can_lose_label(parent_label, child_labels, legal_move_count, compatible_configuration_count, player_to_move, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count)) {
	  validation_failed = true;
	  cout << "[error] FW/CL condition mismatch, configuration_id: " << i << ", parent_label_code: " << parent_label << endl;
	  cout << "child_label_codes: ";
	  for(int j = 0; j < legal_move_count; j++) {
	    for(int m = 0; m < compatible_configuration_count; m++) {
	      cout << child_labels[j][m];
	    }
	    if(j != legal_move_count - 1) cerr << " ";
	  }
	  cout << endl;
	  position.make_posi(i, *zdd, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count);
	  position.print();
	  break;
	}
      }
      if(parent_label == v_lose) {
	if(has_invalid_loss_label(parent_label, child_labels, reversed_view_labels, legal_move_count, compatible_configuration_count, player_to_move, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count)) {
	  validation_failed = true;
	  cout << "[error] FL condition mismatch, configuration_id: " << i << ", parent_label_code: " << parent_label << endl;
	  cout << "child_label_codes: ";
	  if(player_to_move == 1) {
	    for(int j = 0; j < legal_move_count; j++) {
	      for(int m = 0; m < compatible_configuration_count; m++) {
		cout << child_labels[j][m];
	      }
	      if(j != legal_move_count - 1) cerr << " ";
	    }
	  } else {
	    for(int m = 0; m < compatible_configuration_count; m++) {
	      cout << reversed_view_labels[m];
	    }
	  }
	  cout << endl;
	  position.make_posi(i, *zdd, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count);
	  position.print();
	  break;
	}
      }
      if(i % 100000000 == 0) cout << "[progress] Label checks passed, configuration_id: " << i << endl;
    }
  }
  if(validation_failed) terminate();
  return 0;
}
