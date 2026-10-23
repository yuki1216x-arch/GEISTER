#include "node.hpp"

// Check that piece bits agree with the associated count contributions.
bool NodeObjectInfo::is_valid() const noexcept {
  // zero piece
  if((m_piece & COLOR_MASK) == empty) {
    if((m_piece & PLAYER_MASK) != self) return false;
    if(m_blue_count != 0) return false;
    if(m_red_count != 0) return false;
    if(m_unknown_count != 0) return false; }
  // one piece
  else if((m_piece & COLOR_MASK) == blue) {
    if(m_blue_count != 1) return false;
    if(m_red_count != 0) return false;
    if(m_unknown_count != 0) return false; }
  else if((m_piece & COLOR_MASK) == red) {
    if(m_blue_count != 0) return false;
    if(m_red_count != 1) return false;
    if(m_unknown_count != 0) return false; }
  else if((m_piece & COLOR_MASK) == unknown) {
    if((m_piece & PLAYER_MASK) != enemy) return false;
    if(m_blue_count != 0) return false;
    if(m_red_count != 0) return false;
    if(m_unknown_count != 1) return false; }

  return true;
}

// Object IDs: empty, unknown opponent, self blue/red, opponent blue/red.
const NodeObjectInfo node_object_info_by_id[6] =
  { NodeObjectInfo((empty|self),    0, 0, 0),   // 0
    NodeObjectInfo((unknown|enemy), 0, 0, 1),   // 1
    NodeObjectInfo((blue|self),     1, 0, 0),   // 2
    NodeObjectInfo((red|self),      0, 1, 0),   // 3
    NodeObjectInfo((blue|enemy),    1, 0, 0),   // 4
    NodeObjectInfo((red|enemy),     0, 1, 0)    // 5
  };

Node::Node(Node& parent, int branch) noexcept {
  // Advance to the next candidate state, or to the next square after state 3.
  if(parent.m_object_state_id == 3) {
    m_square_index = parent.m_square_index + 1, m_object_state_id = 0;
  } else {
    m_square_index = parent.m_square_index, m_object_state_id = parent.m_object_state_id + 1;
  }

  // A 1-branch selects this candidate; retain that assignment until the square changes.
  if(parent.m_object_state_id != 3 && (parent.m_state_code == 1 || branch == 1)) m_state_code = 1;
  else m_state_code = 0;

  // Only the 1-branch contributes the selected object's piece counts.
  if(branch == 1) {
    m_blue_count = parent.m_blue_count + node_object_info_by_id[parent.m_object_state_id].get_blue_count();
    m_red_count = parent.m_red_count + node_object_info_by_id[parent.m_object_state_id].get_red_count();
    m_unknown_count = parent.m_unknown_count + node_object_info_by_id[parent.m_object_state_id].get_unknown_count();
  } else {
    m_blue_count = parent.m_blue_count, m_red_count = parent.m_red_count, m_unknown_count = parent.m_unknown_count;
  }

  m_configuration_count = 0, m_visited = false;
  m_max_path_length = 0, m_min_path_length = 0, m_total_path_length = 0;
}

bool Node::branch_leads_to_zero_terminal(int depth, int branch, int required_blue_count, int required_red_count, int required_unknown_count) const noexcept {
  assert(0 <= depth && depth < 144);
  assert(branch == 0 || branch == 1);

  // Reject unassigned squares, duplicate assignments, or impossible remaining counts.
  if(branch == 0) {
    // All four candidates were rejected, leaving this square unassigned.
    if(m_object_state_id == 3 && m_state_code == 0) return true;
    if(m_object_state_id == 0 && (36 - m_square_index) - (required_blue_count - m_blue_count) - (required_red_count - m_red_count) - (required_unknown_count - m_unknown_count) < 0) return true;
  } else {
    // One candidate has already been selected for this square.
    if(m_state_code == 1) return true;
    if(required_unknown_count - m_unknown_count - node_object_info_by_id[m_object_state_id].get_unknown_count() < 0) return true;
    if(required_blue_count - m_blue_count - node_object_info_by_id[m_object_state_id].get_blue_count() < 0) return true;
    if(required_red_count - m_red_count - node_object_info_by_id[m_object_state_id].get_red_count() < 0) return true;

    if(m_object_state_id == 0 && (36 - (m_square_index + 1)) - (required_blue_count - m_blue_count) - (required_red_count - m_red_count) - (required_unknown_count - m_unknown_count) < 0) return true;
  }

  return false;
}
