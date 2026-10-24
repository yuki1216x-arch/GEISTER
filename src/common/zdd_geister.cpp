#include <deque>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <map>
#include <stdint.h>
#include <climits>
#include <exception>
#include "node.hpp"
#include "zdd_geister.hpp"

using std::move;

// Local object-count metadata; currently unused by diagram construction.
struct ZddObjectInfo {
  uint8_t piece;
  unsigned char blue_count;
  unsigned char red_count;
  unsigned char unknown_count;

  bool has_same_counts_and_piece(const ZddObjectInfo &other) const noexcept {
    return (piece == other.piece && blue_count == other.blue_count && red_count == other.red_count && unknown_count == other.unknown_count);
  }
};

constexpr ZddObjectInfo zdd_object_info_by_id[6] = {
     {(empty|self),    0, 0, 0},   //0
     {(unknown|enemy), 0, 0, 1},   //1
     {(blue|self),     1, 0, 0},   //2
     {(red|self),      0, 1, 0},   //3
     {(blue|enemy),    1, 0, 0},   //4
     {(red|enemy),     0, 1, 0}    //5
};					  

// Enumerate node states by depth, share equivalent states, then reduce the diagram.
void ZDD::build_diagram(int required_blue_count, int required_red_count, int required_unknown_count) noexcept {
  unique_ptr<Node> root = make_unique<Node>();   // root node
  int depth = -1;   // four candidate states per square
  Node* current_node;

  m_nodes_by_depth[0].push_back(move(root));
  // Expand each square's four candidate object states without changing branch order.
  for(int i = 0; i < 36; i++) {
    for(int j = 0; j < 4; j++) {
      depth++;
      for(size_t k = 0; k < m_nodes_by_depth[depth].size(); k++) {
	current_node = m_nodes_by_depth[depth][k].get();
	
	for(int branch = 0; branch < 2; branch++) {
	  
	  if(current_node->branch_leads_to_zero_terminal(depth, branch, required_blue_count, required_red_count, required_unknown_count)) {
	    if(branch == 0) current_node->m_left = m_zero_terminal.get();
	    else current_node->m_right = m_zero_terminal.get();
	  } else if(depth == 143) {
	    if(branch == 0) current_node->m_left = m_one_terminal.get();
	    else current_node->m_right = m_one_terminal.get();
	  } else {
	    unique_ptr<Node> child_node = make_unique<Node>(*current_node, branch);
	    
	    if(m_nodes_by_depth[depth+1].size() == 0) {     
	      if(branch == 0) current_node->m_left = child_node.get();
	      else current_node->m_right = child_node.get();
	      m_nodes_by_depth[depth+1].push_back(move(child_node));
	    } else {

	      for(size_t l = 0; l < m_nodes_by_depth[depth+1].size(); l++) {
		if(m_nodes_by_depth[depth+1][l]->has_equivalent_state(*child_node)) {
		  if(branch == 0) current_node->m_left = m_nodes_by_depth[depth+1][l].get();
		  else current_node->m_right = m_nodes_by_depth[depth+1][l].get();
		  break;
		} else if(l == m_nodes_by_depth[depth+1].size() - 1) {
		  if(branch == 0) current_node->m_left = child_node.get();
		  else current_node->m_right = child_node.get();
		  m_nodes_by_depth[depth+1].push_back(move(child_node));
		  break;
		}
	      }
            }   
	  }
	}
      }
    }
  }

  // Bypas nodes whose one branch leads to the zero terminal.
  int reduction_changed = 1;
  
  while(reduction_changed == 1){
    reduction_changed = 0;
    for(int i = 0; i < 143; i++){
      for(size_t j = 0; j < m_nodes_by_depth[i].size(); j++){
	if(!m_nodes_by_depth[i][j]->m_left->is_zero_terminal() && !m_nodes_by_depth[i][j]->m_left->is_one_terminal() && !m_nodes_by_depth[i][j]->is_redundant()){
	  if(m_nodes_by_depth[i][j]->m_left->m_right->is_zero_terminal()){
	    reduction_changed = 1;
	    m_nodes_by_depth[i][j]->m_left->mark_redundant();
	    m_nodes_by_depth[i][j]->m_left = m_nodes_by_depth[i][j]->m_left->m_left;
	  }
	}
	if(!m_nodes_by_depth[i][j]->m_right->is_zero_terminal() && !m_nodes_by_depth[i][j]->m_right->is_one_terminal() && !m_nodes_by_depth[i][j]->is_redundant()){
	  if(m_nodes_by_depth[i][j]->m_right->m_right->is_zero_terminal()){
	    reduction_changed = 1;
	    m_nodes_by_depth[i][j]->m_right->mark_redundant();
	    m_nodes_by_depth[i][j]->m_right = m_nodes_by_depth[i][j]->m_right->m_left;
	  }
	}
      }
    }
  }
  // Delete bypassed nodes after all incoming links have been redirected.
  for(int i = 0; i < 144; i++){
    for(size_t j = 0; j < m_nodes_by_depth[i].size(); j++){
      if(m_nodes_by_depth[i][j]->is_redundant()){
	m_nodes_by_depth[i].erase(m_nodes_by_depth[i].begin() + j);
	j--;
      }
    }
  }

  // Count configurations and accumulate root-to-terminal path lengths.
  m_nodes_by_depth[0][0]->compute_subtree_statistics();

  cout << "[zdd] configuration_count: " << m_nodes_by_depth[0][0]->get_configuration_count() << endl;
  cout << "[zdd] max_path_length: " << m_nodes_by_depth[0][0]->get_max_path_length() << endl;
  cout << "[zdd] min_path_length: " << m_nodes_by_depth[0][0]->get_min_path_length() << endl;
  cout << "[zdd] mean_path_length: " << (double)m_nodes_by_depth[0][0]->get_total_path_length() / (double)m_nodes_by_depth[0][0]->get_configuration_count() << endl;

  int node_count = 0;

  for(int i = 0; i < 144; i++){
    node_count += m_nodes_by_depth[i].size();
  }
  cout << "[zdd] node_count: " << node_count << endl;
}
