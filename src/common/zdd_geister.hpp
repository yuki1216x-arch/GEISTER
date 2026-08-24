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

//ZDDのクラス
class ZDD {
private:
  unique_ptr<Node> l0;
  unique_ptr<Node> l1;
  deque<unique_ptr<Node>> m_N[144];  // m_N[i]: set of nodes at depth i
  void construct_zdd(int board_nb, int board_nr, int board_ne) noexcept;
public:
  ZDD(int board_nb, int board_nr, int board_ne) {
    l0 = make_unique<Node>(50, 0, true, -500, 500, 0);
    l1 = make_unique<Node>(60, 1, true, 0, 0, 0);
    construct_zdd(board_nb, board_nr, board_ne);
  }
  ~ZDD() { /* destruct_zdd(); */ }
  // compute the three types of path lengths
  void out_info() const noexcept { 
    cout << "root->num = " << m_N[0][0]->get_num() << endl;
    cout << "root->lengthmax = " << m_N[0][0]->get_lengthmax() << endl;
    cout << "root->lengthmin = " << m_N[0][0]->get_lengthmin() << endl;
    cout << "root->lengthave = " << (double)m_N[0][0]->get_lengthsum() / (double)m_N[0][0]->get_num() << endl;
  }
  void compute_array(unsigned long long int x, unsigned char* array_objid, size_t size) const noexcept {
    assert(size == 36);
    const Node *r = m_N[0][0].get();
    int locid = 0;
    for(size_t i = 0; i < size; i++) {
      array_objid[i] = 6;
    }
    while(true) {
      assert(r);
      if(r->get_f() == 60) break; // if this is a 1-leaf
      assert(r->get_f() != 50 && (r->get_f() == 0 || r->get_f() == 1));

      if(r->m_left->get_num() <= x) {
	x -= r->m_left->get_num();
	assert(r->get_locid() == locid);
	assert(r->get_locid() >= 0 && r->get_locid() <= static_cast<int>(size));
	array_objid[static_cast<size_t>(r->get_locid())] = static_cast<unsigned char>(r->get_loc_stateid());
	locid++;
	r = r->m_right;
      } else {
	r = r->m_left;
      }
    }
  }
  unsigned long long int compute_id(unsigned char* array_objid, std::size_t size) const noexcept {
    assert(size == 36);
    const Node* r = m_N[0][0].get();
    unsigned long long int index = 0;
    while(r->get_f() == 0 || r->get_f() == 1) {
      assert(r->get_locid() >= 0 && r->get_locid() < static_cast<int>(size));
      if(array_objid[static_cast<size_t>(r->get_locid())] == static_cast<unsigned char>(r->get_loc_stateid())) {
	index += r->m_left->get_num();
	r = r->m_right;
      } else {
	r = r->m_left;
      }
    }
    return index;
  }
};
    
#endif
