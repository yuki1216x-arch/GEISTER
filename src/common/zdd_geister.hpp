#ifndef INCLUDE_ZDD_GEISTER_H
#define INCLUDE_ZDD_GEISTER_H

#include <cassert>
#include <bitset>
#include <stdint.h>
#include <cstddef>
#include <deque>
#include <vector>
#include <algorithm>
#include <iostream>
#include <exception>
#include <memory>
#include "node.hpp"

using std::deque;
using std::cout;
using std::endl;
using std::cerr;
using std::terminate;
using std::unique_ptr;
using std::make_unique;
using std::vector;
using std::size_t;

// Map Geister board configurations to IDs using a zero-supporessed decision diagram.
// Board object IDs and traversal order determine the database entry order.
class ZDD {
private:
  unique_ptr<Node> m_zero_terminal;
  unique_ptr<Node> m_one_terminal;
  deque<unique_ptr<Node>> m_nodes_by_depth[144];   // owned nodes grouped by depth
  void build_diagram(int required_blue_count, int required_red_count, int required_unknown_count) noexcept;
public:
  ZDD(int required_blue_count, int required_red_count, int required_unknown_count) {
    m_zero_terminal = make_unique<Node>(50, 0, true, -500, 500, 0);
    m_one_terminal = make_unique<Node>(60, 1, true, 0, 0, 0);
    build_diagram(required_blue_count, required_red_count, required_unknown_count);
  }
  ~ZDD() {}
  
  // Print configuration counts and root-to-terminal path statistics.
  void print_statistics() const noexcept { 
    cout << "[zdd] configuration_count: " << m_nodes_by_depth[0][0]->get_configuration_count() << endl;
    cout << "[zdd] max_path_length: " << m_nodes_by_depth[0][0]->get_max_path_length() << endl;
    cout << "[zdd] min_path_length: " << m_nodes_by_depth[0][0]->get_min_path_length() << endl;
    cout << "[zdd] mean_path_length: " << (double)m_nodes_by_depth[0][0]->get_total_path_length() / (double)m_nodes_by_depth[0][0]->get_configuration_count() << endl;
  }
  // Decode a valid ID into 36 board object IDs, in square-index order.
  void decode_configuration(unsigned long long int remaining_configuration_id, unsigned char* board_object_ids, size_t size) const noexcept {
    assert(size == 36);
    const Node *current_node = m_nodes_by_depth[0][0].get();
    int assigned_square_count = 0;
    for(size_t i = 0; i < size; i++) {
      board_object_ids[i] = 6;
    }
    // Skip the zero branch count when the ID belongs to the one branch.
    while(true) {
      assert(current_node);
      if(current_node->get_state_code() == 60) break;
      assert(current_node->get_state_code() != 50 && (current_node->get_state_code() == 0 || current_node->get_state_code() == 1));

      if(current_node->m_left->get_configuration_count() <= remaining_configuration_id) {
	remaining_configuration_id -= current_node->m_left->get_configuration_count();
	assert(current_node->get_square_index() == assigned_square_count);
	assert(current_node->get_square_index() >= 0 && current_node->get_square_index() <= static_cast<int>(size));
	board_object_ids[static_cast<size_t>(current_node->get_square_index())] = static_cast<unsigned char>(current_node->get_object_state_id());
	assigned_square_count++;
	current_node = current_node->m_right;
      } else {
	current_node = current_node->m_left;
      }
    }
  }
  // Encode a valid 36-square board belonging to this piece-count class.
  unsigned long long int encode_configuration(unsigned char* board_object_ids, std::size_t size) const noexcept {
    assert(size == 36);
    const Node* current_node = m_nodes_by_depth[0][0].get();
    unsigned long long int configuration_id = 0;
    // Add configurations skipped whenever the selected object tasks the one branch.
    while(current_node->get_state_code() == 0 || current_node->get_state_code() == 1) {
      assert(current_node->get_square_index() >= 0 && current_node->get_square_index() < static_cast<int>(size));
      if(board_object_ids[static_cast<size_t>(current_node->get_square_index())] == static_cast<unsigned char>(current_node->get_object_state_id())) {
	configuration_id += current_node->m_left->get_configuration_count();
	current_node = current_node->m_right;
      } else {
	current_node = current_node->m_left;
      }
    }
    return configuration_id;
  }
};
    
#endif
