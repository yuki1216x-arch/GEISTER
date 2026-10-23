#ifndef INCLUDE_NODE_H
#define INCLUDE_NODE_H

#include <exception>
#include <memory>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cassert>

using std::cout;
using std::endl;
using std::max;
using std::min;
using std::unique_ptr;

constexpr uint8_t PLAYER_MASK = 0b0001;
constexpr uint8_t COLOR_MASK  = 0b0110;

constexpr uint8_t self    = 0b0000;
constexpr uint8_t enemy   = 0b0001;
constexpr uint8_t empty   = 0b0000;
constexpr uint8_t blue    = 0b0010;
constexpr uint8_t red     = 0b0100;
constexpr uint8_t unknown = 0b0110;

// Piece encoding and count contributions for one candidate square state.
class NodeObjectInfo {
  uint8_t m_piece;
  int m_blue_count;
  int m_red_count;
  int m_unknown_count;

public:
  NodeObjectInfo(char piece, char blue_count, char red_count, char unknown_count) noexcept: m_piece(piece), m_blue_count(blue_count), m_red_count(red_count), m_unknown_count(unknown_count) { assert(is_valid()); }
  int get_piece() const noexcept { return m_piece; }
  int get_blue_count() const noexcept { return m_blue_count; }
  int get_red_count() const noexcept { return m_red_count; }
  int get_unknown_count() const noexcept { return m_unknown_count; }
  bool is_valid() const noexcept;
  bool operator==(const NodeObjectInfo &other) const noexcept {
    assert(other.is_valid());
    return (m_piece == other.m_piece && m_blue_count == other.m_blue_count && m_red_count == other.m_red_count && m_unknown_count == other.m_unknown_count);
  }
};

// ZDD construction state and statistics for the subgraph rooted at this node.
class Node {
  int m_blue_count;
  int m_red_count;
  int m_unknown_count;
  // 0/1: square assignment state; 40: redundant; 50/60: zero/one terminal.
  int m_state_code;
  int m_square_index;
  int m_object_state_id;   // candidate object ID for the current square
  unsigned long long int m_configuration_count;   // paths to the one terminal
  bool m_visited;   // whether subtree statistics have been computed
  int m_max_path_length;
  int m_min_path_length;
  unsigned long long int m_total_path_length;

public:
  Node()
    : m_blue_count(0), m_red_count(0), m_unknown_count(0), m_state_code(0),
      m_square_index(0), m_object_state_id(0), m_configuration_count(0), m_visited(false),
      m_max_path_length(0), m_min_path_length(0), m_total_path_length(0) {}
  Node(char state_code, unsigned long long int configuration_count, bool statistics_computed, int max_path_length, int min_path_length, unsigned long long int total_path_length)
    : m_blue_count(state_code), m_red_count(state_code), m_unknown_count(state_code), m_state_code(state_code),
      m_square_index(0), m_object_state_id(0), m_configuration_count(configuration_count), m_visited(statistics_computed),
      m_max_path_length(max_path_length), m_min_path_length(min_path_length), m_total_path_length(total_path_length) {}
  // Construct the state reached by taking a parent's 0- or 1-branch.
  Node(Node& parent, int branch) noexcept;
  // Non-owning branch pointers; the ZDD owns nodes and terminal objects.
  Node* m_left;
  Node* m_right;

  // Return true when the branch violates square assignment or piece-count constraints.
  bool branch_leads_to_zero_terminal(int depth, int branch, int required_blue_count, int required_red_count, int required_unknown_count) const noexcept;
  int get_square_index() const noexcept { return m_square_index; }
  int get_object_state_id() const noexcept { return m_object_state_id; }
  unsigned long long int get_configuration_count() const noexcept { return m_configuration_count; }
  int get_state_code() const noexcept { return m_state_code; }
  int get_max_path_length() const noexcept { return m_max_path_length; }
  int get_min_path_length() const noexcept { return m_min_path_length; }
  unsigned long long int get_total_path_length() const noexcept { return m_total_path_length; }
  // Aggregate configuration counts and path lengths after child statistics are available.
  void compute_subtree_statistics() noexcept {
    assert(m_left != nullptr && m_right != nullptr);
    if(!m_left->m_visited) m_left->compute_subtree_statistics();
    if(!m_right->m_visited) m_right->compute_subtree_statistics();
    m_configuration_count = m_left->m_configuration_count + m_right->m_configuration_count;
    m_max_path_length = max(m_left->m_max_path_length, m_right->m_max_path_length) + 1;
    m_min_path_length = min(m_left->m_min_path_length, m_right->m_min_path_length) + 1;
    m_total_path_length = m_left->m_total_path_length + m_right->m_total_path_length + m_configuration_count;
    m_visited = true;
  }
  // Compare assignment atate and piece counts within the same ZDD depth.
  bool has_equivalent_state(Node& other) const noexcept {
    return m_state_code == other.m_state_code && m_blue_count == other.m_blue_count && m_red_count == other.m_red_count && m_unknown_count == other.m_unknown_count;
  }
  void mark_redundant() noexcept { m_state_code = 40; }
  bool is_redundant() const noexcept { return m_state_code == 40; }
  bool is_zero_terminal() const noexcept { return m_state_code == 50; }
  bool is_one_terminal() const noexcept { return m_state_code == 60; }
};

#endif
