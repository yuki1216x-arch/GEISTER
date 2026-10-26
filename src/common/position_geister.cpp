#include <deque>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <map>
#include <stdint.h>
#include <climits>
#include <exception>
#include <functional>
#include <iostream>
#include <vector>
#include "posi_geister.hpp"

using std::copy;
using std::function;
using namespace std;

// Enumerate index combinations in descending candidate order.
void enumerate_combinations_recursive(int *selected_indices, int candidate_index, int remaining_count, function<void(int *)> callback) {
  if(remaining_count == 0) {
    callback(selected_indices);
  } else {
    if(candidate_index < 0) return;
    enumerate_combinations_recursive(selected_indices, candidate_index - 1, remaining_count, callback);
    selected_indices[remaining_count - 1] = candidate_index;
    enumerate_combinations_recursive(selected_indices, candidate_index - 1, remaining_count - 1, callback);
  }
}

// Invoke the callback once per n-choose-k combination.
void for_each_combination(int n, int k, function<void(int *)> callback) {
  int selected_indices[k];
  enumerate_combinations_recursive(selected_indices, n - 1, k, callback);
}

Position::Position() noexcept {
  for(int square_index = 0; square_index < 36; square_index++) m_board[square_index] = square_info_by_object_id[0];
}

Position::Position(unsigned long long int configuration_id, const ZDD& zdd,
	   int self_blue_count, int self_red_count, int opponent_blue_count, int opponent_red_count) noexcept {
  unsigned char board_object_ids[36];
  zdd.decode_configuration(configuration_id, board_object_ids, 36);
  for(int square_index = 0; square_index < 36; square_index++) m_board[square_index] = square_info_by_object_id[board_object_ids[square_index]];
  m_self_blue_count = self_blue_count;
  m_self_red_count = self_red_count;
  m_opponent_blue_count = opponent_blue_count;
  m_opponent_red_count = opponent_red_count;
}

void Position::load_configuration(unsigned long long int configuration_id, const ZDD& zdd,
		     int self_blue_count, int self_red_count, int opponent_blue_count, int opponent_red_count) noexcept {
  unsigned char board_object_ids[36];
  zdd.decode_configuration(configuration_id, board_object_ids, 36);
  for(int square_index = 0; square_index < 36; square_index++) m_board[square_index] = square_info_by_object_id[board_object_ids[square_index]];
  m_self_blue_count = self_blue_count;
  m_self_red_count = self_red_count;
  m_opponent_blue_count = opponent_blue_count;
  m_opponent_red_count = opponent_red_count;
}

void Position::load_compatible_configuration(unsigned char compatible_boards[70][36], int n) noexcept {
  for(int square_index = 0; square_index < 36; square_index++) {
    assert(compatible_boards[n][square_index] < 6);
    m_board[square_index] = square_info_by_object_id[compatible_boards[n][square_index]];
  }
}

void Position::switch_to_opponent_view() noexcept {
  for(int square_index = 0; square_index < 36; square_index++) {
    assert(m_board[square_index].piece != (unknown|enemy));
    if(m_board[square_index].piece == (blue|self)) m_board[square_index] = square_info_by_object_id[4];
    else if(m_board[square_index].piece == (red|self)) m_board[square_index] = square_info_by_object_id[5];
    else if(m_board[square_index].piece == (blue|enemy)) m_board[square_index] = square_info_by_object_id[2];
    else if(m_board[square_index].piece == (red|enemy)) m_board[square_index] = square_info_by_object_id[3];
    else assert(m_board[square_index].piece == (empty|self));
  }
  swap(m_self_blue_count, m_opponent_blue_count);
  swap(m_self_red_count, m_opponent_red_count);

  // Rotate the board 180 degrees after swapping ownership and piece counts.
  for(int square_index = 0; square_index < 18; square_index++) swap(m_board[square_index], m_board[35 - square_index]);

  for(int square_index = 0; square_index < 36; square_index++) {
    if(m_board[square_index].piece == (blue|enemy) || m_board[square_index].piece == (red|enemy)) m_board[square_index] = square_info_by_object_id[1];
  }
}

void Position::hide_opponent_colors() noexcept {
  for(int square_index = 0; square_index < 36; square_index++) {
    if(m_board[square_index].piece == (blue|enemy) || m_board[square_index].piece == (red|enemy)) m_board[square_index] = square_info_by_object_id[1];
  }
}

void Position::print() const noexcept {
  cout << "self (blue: " << m_self_blue_count << ", red: " << m_self_red_count << ")" << endl;
  cout << "opponent (blue: " << m_opponent_blue_count << ", red: " << m_opponent_red_count << ")" << endl;
  cout << endl;
  cout << "   _______________\n";
  for(int i = 0; i < 6; i++) {
    cout << "   | ";
    for(int j = 0; j < 6; j++) {
      cout << m_board[35 - 6 * i - j].display_character << " ";
    }
    cout << "|\n";
  }
  cout << "   ^^^^^^^^^^^^^^^" << endl;
}

int Position::generate_legal_actions(Action actions[1000], int turn_selector) noexcept {
  int legal_move_count = 0;
  // Preserve move ordering and the parity-based choice of the moving player.
  if(turn_selector % 2 == 1) {
    if(m_board[30].piece == (blue|self)) actions[legal_move_count] = Action(30, 100), legal_move_count++;
    else if(m_board[35].piece == (blue|self)) actions[legal_move_count] = Action(35, 100), legal_move_count++;
    
    for(int position_id = 0; position_id < 36; position_id++) {
      if((m_board[position_id].piece & PLAYER_MASK) == self && (m_board[position_id].piece & COLOR_MASK) != empty) {
	
	if(position_id >= 6) {
	  if((m_board[position_id - 6].piece & PLAYER_MASK) != self || (m_board[position_id - 6].piece & COLOR_MASK) == empty) {
	    actions[legal_move_count] = Action(position_id, position_id - 6);
	    legal_move_count++;
	  }
	}

	if(position_id < 30) {
	  if((m_board[position_id + 6].piece & PLAYER_MASK) != self || (m_board[position_id + 6].piece & COLOR_MASK) == empty) {
	    actions[legal_move_count] = Action(position_id, position_id + 6);
	    legal_move_count++;
	  }
	}

	if(position_id % 6 != 0) {
	  if((m_board[position_id - 1].piece & PLAYER_MASK) != self || (m_board[position_id - 1].piece & COLOR_MASK) == empty) {
	    actions[legal_move_count] = Action(position_id, position_id - 1);
	    legal_move_count++;
	  }
	}

	if(position_id % 6 != 5) {
	  if((m_board[position_id + 1].piece & PLAYER_MASK) != self || (m_board[position_id + 1].piece & COLOR_MASK) == empty) {
	    actions[legal_move_count] = Action(position_id, position_id + 1);
	    legal_move_count++;
	  }
	}
      }
    }
  } else {
    assert(turn_selector % 2 == 0);
    if(m_board[0].piece == (unknown|enemy) || m_board[0].piece == (blue|enemy)) actions[legal_move_count] = Action(0, 100), legal_move_count++;
    else if(m_board[5].piece == (unknown|enemy) || m_board[5].piece == (blue|enemy)) actions[legal_move_count] = Action(5, 100), legal_move_count++;
    
    for(int position_id = 0; position_id < 36; position_id++) {
      if((m_board[position_id].piece & PLAYER_MASK) == enemy && (m_board[position_id].piece & COLOR_MASK) != empty) {

	if(position_id >= 6) {
	  if((m_board[position_id - 6].piece & PLAYER_MASK) != enemy || (m_board[position_id - 6].piece & COLOR_MASK) == empty) {
	    actions[legal_move_count] = Action(position_id, position_id - 6);
	    legal_move_count++;
	  }
	}

	if(position_id < 30) {
	  if((m_board[position_id + 6].piece & PLAYER_MASK) != enemy || (m_board[position_id + 6].piece & COLOR_MASK) == empty) {
	    actions[legal_move_count] = Action(position_id, position_id + 6);
	    legal_move_count++;
	  }
	}

	if(position_id % 6 != 0) {
	  if((m_board[position_id - 1].piece & PLAYER_MASK) != enemy || (m_board[position_id - 1].piece & COLOR_MASK) == empty) {
	    actions[legal_move_count] = Action(position_id, position_id - 1);
	    legal_move_count++;
	  }
	}

	if(position_id % 6 != 5) {
	  if((m_board[position_id + 1].piece & PLAYER_MASK) != enemy || (m_board[position_id + 1].piece & COLOR_MASK) == empty) {
	    actions[legal_move_count] = Action(position_id, position_id + 1);
	    legal_move_count++;
	  }
	}
      }
    }
  }
  
  assert(legal_move_count < 32);
  return legal_move_count;
}

int Position::get_object_id(const SquareInfo& square) const noexcept {
  int object_id = -1;
  if(square.piece == (empty|self)) {
    object_id = 0;
  } else if(square.piece == (unknown|enemy)) {
    object_id = 1;
  } else if(square.piece == (blue|self)) {
    object_id = 2;
  } else if(square.piece == (red|self)) {
    object_id = 3;
  } else if(square.piece == (blue|enemy)) {
    object_id = 4;
  } else if(square.piece == (red|enemy)) {
    object_id = 5;
  }

  assert(object_id != -1);
  return object_id;
}

unsigned long long int Position::get_zdd_id(const ZDD& zdd) const noexcept {
  unsigned char array[36] = {};
  for(int i = 0; i < 36; i++) {
    array[i] = get_object_id(m_board[i]);
  }
  return zdd.encode_configuration(array, 36);
}

int Position::enumerate_compatible_configurations(unsigned char compatible_boards[70][36]) const noexcept {
  int compatible_configuration_count = 0;
  int opponent_blue_count = 0, unknown_count = 0;

  // Resolve each unknown opponent piece according to the chosen blue-piece indices.
  for_each_combination
    (m_opponent_blue_count + m_opponent_red_count, m_opponent_blue_count, [&](int *indexes)
      {
	opponent_blue_count = 0, unknown_count = 0;
	for(int i = 0; i < 36; i++) {
	  if((m_board[i].piece & PLAYER_MASK) == enemy) {
	    assert(m_board[i].piece == (unknown|enemy));
	    if(opponent_blue_count < m_opponent_blue_count && indexes[opponent_blue_count] == unknown_count) {
	      compatible_boards[compatible_configuration_count][i] = 4;
	      opponent_blue_count++;
	      assert(opponent_blue_count <= m_opponent_blue_count);
	    } else {
	      compatible_boards[compatible_configuration_count][i] = 5;
	    }
	    unknown_count++;
	    assert(unknown_count <= m_opponent_blue_count + m_opponent_red_count);
	  } else {
	    compatible_boards[compatible_configuration_count][i] = get_object_id(m_board[i]);
	  }
	}
	compatible_configuration_count++;
	assert(compatible_configuration_count > 0 && compatible_configuration_count <= 70);
      });
  
  return compatible_configuration_count;
}

int Position::apply_action(const Action& action) noexcept {
  // Save the complete board and piece counts before any change.
  Snapshot snapshot;
  copy(m_board, m_board + 36, snapshot.m_board);
  snapshot.m_self_blue_count = m_self_blue_count;
  snapshot.m_self_red_count = m_self_red_count;
  snapshot.m_opponent_blue_count = m_opponent_blue_count;
  snapshot.m_opponent_red_count = m_opponent_red_count;
  m_history.push(snapshot);

  int source_square = action.get_source_square();
  int destination_square = action.get_destination_square();
  assert(source_square >= 0 && source_square < 36);

  // An escape is represented by the off-board destination 100.
  if(destination_square == 100) {
    int result;
    if(source_square == 30 || source_square == 35) result = -1;
    else {
      assert(source_square == 0 || source_sqaure == 5);
#ifdef USE_PURPLE
      assert(m_board[source_square].piece == (unknown|enemy));
      result = -2;
#else
      if(m_board[source_square].piece == (blue|enemy)) result = -2;
      else {
	assert(m_board[source_square].piece == (red|enemy));
	result = -1;
      }
#endif
    }
    m_board[source_square] = square_info_by_object_id[0];
    return result;
  }

  int result = 0;
  assert(destination_square >= 0 && destination_square < 36);
  if((m_board[destination_square].piece & COLOR_MASK) != empty) {
    char captured_piece = m_board[destination_square].piece;
    if((captured_piece & PLAYER_MASK) == self) {
      if(captured_piece == (blue|self)) {
	m_self_blue_count--;
	result = 1;
      } else {
	assert(captured_piece == (red|self));
	m_self_red_count--;
	result = 2;
      }
    } else {
      assert((captured_piece & PLAYER_MASK) == enemy);
#ifdef USE_PURPLE
      assert(captured_piece == (unknown|enemy));
      m_opponent_red_count--;
      result = 2;
#else
      if(captured_piece == (blue|enemy)) {
	m_opponent_blue_count--;
	result = 1;
      } else {
	assert(captured_piece == (red|enemy));
	m_opponent_red_count--;
	result = 2;
      }
#endif
    }
  }
  m_board[destination_square] = m_board[source_square];
  m_board[source_square] = square_info_by_object_id[0];

  // Terminal results take priority over the captured-piece code.
  if(m_opponent_blue_count == 0 || m_self_red_count == 0) return -1;
  if(m_self_blue_count == 0 || m_opponent_red_count == 0) return -2;

  return result;
}

void Position::undo_action() noexcept {
  // Undo exactly one apply_action call.
  Snapshot snapshot = m_history.top();
  m_history.pop();

  copy(snapshot.m_board, snapshot.m_board + 36, m_board);
  m_self_blue_count = snapshot.m_self_blue_count;
  m_self_red_count = snapshot.m_self_red_count;
  m_opponent_blue_count = snapshot.m_opponent_blue_count;
  m_opponent_red_count = snapshot.m_opponent_red_count;
}

vector<string> split(const string text, const char delimiter='/');
