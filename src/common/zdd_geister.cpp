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

struct LocInfo {
  uint8_t piece;
  unsigned char nb;
  unsigned char nr;
  unsigned char uk;

  bool equal(const LocInfo &o) const noexcept {
    return (piece == o.piece && nb == o.nb && nr == o.nr && uk == o.uk);
  }
};

constexpr LocInfo tbl_objid2locinfo[6] = {
     {(empty|self),    0, 0, 0},   //0
     {(unknown|enemy), 0, 0, 1},   //1
     {(blue|self),     1, 0, 0},   //2
     {(red|self),      0, 1, 0},   //3
     {(blue|enemy),    1, 0, 0},   //4
     {(red|enemy),     0, 1, 0}    //5
};					  

// cunstruct a zdd
void ZDD::construct_zdd(int board_nb, int board_nr, int board_ne) noexcept {
  unique_ptr<Node> root = make_unique<Node>();   // root node
  int d = -1;
  Node* n;

  m_N[0].push_back(move(root));   // add the root node to N[0]
  for(int i = 0; i < 36; i++) {
    for(int j = 0; j < 4; j++) {
      d++;
      for(size_t k = 0; k < m_N[d].size(); k++) {
	n = m_N[d][k].get();

	// follow the 0-branch or 1-branch
	for(int x = 0; x < 2; x++) {
	  // check whether the 0-leaf is reached
	  if(n->IsNextLeaf0(d, x, board_nb, board_nr, board_ne)) {  // if the x-branch leads to the 0-leaf
	    if(x == 0) n->m_left = l0.get();
	    else n->m_right = l0.get();
	  } else if(d == 143) {   // if the maximum depth is reached
	    if(x == 0) n->m_left = l1.get();
	    else n->m_right = l1.get();
	  } else {   // if the branch does not lead to the 0-leaf and the maximum depth has not been reached
	    unique_ptr<Node> c = make_unique<Node>(*n, x);   // create a node
	    
	    // if the node does not exist at depth d + 1, register it as a new node
	    if(m_N[d+1].size() == 0) {     
	      if(x == 0) n->m_left = c.get(); // connect the parent and child nodes
	      else n->m_right = c.get();
	      m_N[d+1].push_back(move(c));    // add the node to the set of nodes at the next depth
	    } else {
	      // share equivalent nodes
	      for(size_t l = 0; l < m_N[d+1].size(); l++) {
		if(m_N[d+1][l]->ExistEquivalentNode(*c)) {
		  if(x == 0) n->m_left = m_N[d+1][l].get();
		  else n->m_right = m_N[d+1][l].get();
		  break;
		} else if(l == m_N[d+1].size() - 1) {
		  if(x == 0) n->m_left = c.get();
		  else n->m_right = c.get();
		  m_N[d+1].push_back(move(c));
		  break;
		}
	      }
            }   
	  }
	}
      }
    }
  }
  
  int end = 1;
  // remove redundant nodes
  while(end == 1){
    end = 0;
    for(int i = 0; i < 143; i++){
      for(size_t j = 0; j < m_N[i].size(); j++){
	if(!m_N[i][j]->m_left->IsLeaf0() && !m_N[i][j]->m_left->IsLeaf1() && !m_N[i][j]->IsRedundantNode()){
	  if(m_N[i][j]->m_left->m_right->IsLeaf0()){
	    end = 1;
	    m_N[i][j]->m_left->setRedundantNode();
	    m_N[i][j]->m_left = m_N[i][j]->m_left->m_left;
	  }
	}
	if(!m_N[i][j]->m_right->IsLeaf0() && !m_N[i][j]->m_right->IsLeaf1() && !m_N[i][j]->IsRedundantNode()){
	  if(m_N[i][j]->m_right->m_right->IsLeaf0()){
	    end = 1;
	    m_N[i][j]->m_right->setRedundantNode();
	    m_N[i][j]->m_right = m_N[i][j]->m_right->m_left;
	  }
	}
      }
    }
  }
  for(int i = 0; i < 144; i++){
    for(size_t j = 0; j < m_N[i].size(); j++){
      if(m_N[i][j]->IsRedundantNode()){
	m_N[i].erase(m_N[i].begin() + j);
	j--;
      }
    }
  }

  m_N[0][0]->dfs();

  cout << "root->num = " << m_N[0][0]->get_num() << endl;
  cout << "root->lengthmax = " << m_N[0][0]->get_lengthmax() << endl;
  cout << "root->lengthmin = " << m_N[0][0]->get_lengthmin() << endl;
  cout << "root->lengthave = " << (double)m_N[0][0]->get_lengthsum() / (double)m_N[0][0]->get_num() << endl;

  int sum = 0;

  for(int i = 0; i < 144; i++){
    sum += m_N[i].size();
  }
  cout << "sumad =" << sum << endl;   // number of nodes
}
