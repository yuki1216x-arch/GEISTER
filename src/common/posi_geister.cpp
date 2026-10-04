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

void recursive_comb(int *indexes, int s, int rest, function<void(int *)> f) {
  if (rest == 0) {
    f(indexes);
  } else {
    if (s < 0) return;
    recursive_comb(indexes, s - 1, rest, f);
    indexes[rest - 1] = s;
    recursive_comb(indexes, s - 1, rest - 1, f);
  }
}

// Process all nCk combinations
void foreach_comb(int n, int k, function<void(int *)> f) {
  int indexes[k];
  recursive_comb(indexes, n - 1, k, f);
}

Posi::Posi() noexcept {
  for(int loc = 0; loc < 36; loc++) m_array[loc] = tbl_objid2locinfo[0];
}

Posi::Posi(unsigned long long int x, const ZDD& zdd,
	   int self_blue_count, int self_red_count, int enemy_blue_count, int enemy_red_count) noexcept {
  unsigned char array_objid[36];
  zdd.compute_array(x, array_objid, 36);
  for(int loc = 0; loc < 36; loc++) m_array[loc] = tbl_objid2locinfo[array_objid[loc]];
  m_self_blue_count = self_blue_count;
  m_self_red_count = self_red_count;
  m_enemy_blue_count = enemy_blue_count;
  m_enemy_red_count = enemy_red_count;
}

void Posi::make_posi(unsigned long long int x, const ZDD& zdd,
		     int self_blue_count, int self_red_count, int enemy_blue_count, int enemy_red_count) noexcept {
  unsigned char array_objid[36];
  zdd.compute_array(x, array_objid, 36);
  for(int loc = 0; loc < 36; loc++) m_array[loc] = tbl_objid2locinfo[array_objid[loc]];
  m_self_blue_count = self_blue_count;
  m_self_red_count = self_red_count;
  m_enemy_blue_count = enemy_blue_count;
  m_enemy_red_count = enemy_red_count;
}

void Posi::make_posi_n(unsigned char zdd_code[70][36], int n) noexcept {
  for(int loc = 0; loc < 36; loc++) {
    assert(zdd_code[n][loc] < 6);
    m_array[loc] = tbl_objid2locinfo[zdd_code[n][loc]];
  }
}

void Posi::make_posi_opponent() noexcept {
  // swap the piece owners
  for(int loc = 0; loc < 36; loc++) {
    assert(m_array[loc].piece != (unknown|enemy));
    if(m_array[loc].piece == (blue|self)) m_array[loc] = tbl_objid2locinfo[4];
    else if(m_array[loc].piece == (red|self)) m_array[loc] = tbl_objid2locinfo[5];
    else if(m_array[loc].piece == (blue|enemy)) m_array[loc] = tbl_objid2locinfo[2];
    else if(m_array[loc].piece == (red|enemy)) m_array[loc] = tbl_objid2locinfo[3];
    else assert(m_array[loc].piece == (empty|self));
  }
  swap(m_self_blue_count, m_enemy_blue_count);
  swap(m_self_red_count, m_enemy_red_count);

  // rotate the board 180 degrees
  for(int loc = 0; loc < 18; loc++) swap(m_array[loc], m_array[35 - loc]);

  // make all opponent's pieces unknown
  for(int loc = 0; loc < 36; loc++) {
    if(m_array[loc].piece == (blue|enemy) || m_array[loc].piece == (red|enemy)) m_array[loc] = tbl_objid2locinfo[1];
  }
}

void Posi::make_posi_myself() noexcept {
  // make all opponent's pieces unknown
  for(int loc = 0; loc < 36; loc++) {
    if(m_array[loc].piece == (blue|enemy) || m_array[loc].piece == (red|enemy)) m_array[loc] = tbl_objid2locinfo[1];
  }
}

void Posi::print() const noexcept {
  cout << "self (blue: " << m_self_blue_count << ", red: " << m_self_red_count << ")" << endl;
  cout << "enemy (blue: " << m_enemy_blue_count << ", red: " << m_enemy_red_count << ")" << endl;
  cout << endl;
  cout << "   _______________\n";
  for(int i = 0; i < 6; i++) {
    cout << "   | ";
    for(int j = 0; j < 6; j++) {
      cout << m_array[35 - 6 * i - j].piece_ch << " ";
    }
    cout << "|\n";
  }
  cout << "   ^^^^^^^^^^^^^^^" << endl;
}
int Posi::compute_actions(Action actions[1000], int iter) noexcept {
  int naction = 0; // number of legal moves
  if(iter % 2 == 1) {
    if(m_array[30].piece == (blue|self)) actions[naction] = Action(30, 100), naction++;
    else if(m_array[35].piece == (blue|self)) actions[naction] = Action(35, 100), naction++;
    
    for(int position_id = 0; position_id < 36; position_id++) {
      if((m_array[position_id].piece & PLAYER_MASK) == self && (m_array[position_id].piece & COLOR_MASK) != empty) {
	// move down
	if(position_id >= 6) {
	  if((m_array[position_id - 6].piece & PLAYER_MASK) != self || (m_array[position_id - 6].piece & COLOR_MASK) == empty) {
	    actions[naction] = Action(position_id, position_id - 6);
	    naction++;
	  }
	}
	// move up
	if(position_id < 30) {
	  if((m_array[position_id + 6].piece & PLAYER_MASK) != self || (m_array[position_id + 6].piece & COLOR_MASK) == empty) {
	    actions[naction] = Action(position_id, position_id + 6);
	    naction++;
	  }
	}
	// move right
	if(position_id % 6 != 0) {
	  if((m_array[position_id - 1].piece & PLAYER_MASK) != self || (m_array[position_id - 1].piece & COLOR_MASK) == empty) {
	    actions[naction] = Action(position_id, position_id - 1);
	    naction++;
	  }
	}
	// move left
	if(position_id % 6 != 5) {
	  if((m_array[position_id + 1].piece & PLAYER_MASK) != self || (m_array[position_id + 1].piece & COLOR_MASK) == empty) {
	    actions[naction] = Action(position_id, position_id + 1);
	    naction++;
	  }
	}
      }
    }
  } else {
    assert(iter % 2 == 0);
    if(m_array[0].piece == (unknown|enemy) || m_array[0].piece == (blue|enemy)) actions[naction] = Action(0, 100), naction++;
    else if(m_array[5].piece == (unknown|enemy) || m_array[5].piece == (blue|enemy)) actions[naction] = Action(5, 100), naction++;
    
    for(int position_id = 0; position_id < 36; position_id++) {
      if((m_array[position_id].piece & PLAYER_MASK) == enemy && (m_array[position_id].piece & COLOR_MASK) != empty) {
	// move down
	if(position_id >= 6) {
	  if((m_array[position_id - 6].piece & PLAYER_MASK) != enemy || (m_array[position_id - 6].piece & COLOR_MASK) == empty) {
	    actions[naction] = Action(position_id, position_id - 6);
	    naction++;
	  }
	}
	// move up
	if(position_id < 30) {
	  if((m_array[position_id + 6].piece & PLAYER_MASK) != enemy || (m_array[position_id + 6].piece & COLOR_MASK) == empty) {
	    actions[naction] = Action(position_id, position_id + 6);
	    naction++;
	  }
	}
	// move right
	if(position_id % 6 != 0) {
	  if((m_array[position_id - 1].piece & PLAYER_MASK) != enemy || (m_array[position_id - 1].piece & COLOR_MASK) == empty) {
	    actions[naction] = Action(position_id, position_id - 1);
	    naction++;
	  }
	}
	// move left
	if(position_id % 6 != 5) {
	  if((m_array[position_id + 1].piece & PLAYER_MASK) != enemy || (m_array[position_id + 1].piece & COLOR_MASK) == empty) {
	    actions[naction] = Action(position_id, position_id + 1);
	    naction++;
	  }
	}
      }
    }
  }
  
  assert(naction < 32);
  return naction;
}

int Posi::getobjnum(const LocInfo& a) const noexcept {
  int obj_id = -1;
  if(a.piece == (empty|self)) {
    obj_id = 0;
  } else if(a.piece == (unknown|enemy)) {
    obj_id = 1;
  } else if(a.piece == (blue|self)) {
    obj_id = 2;
  } else if(a.piece == (red|self)) {
    obj_id = 3;
  } else if(a.piece == (blue|enemy)) {
    obj_id = 4;
  } else if(a.piece == (red|enemy)) {
    obj_id = 5;
  }

  assert(obj_id != -1);
  return obj_id;
}

unsigned long long int Posi::getzddnum(const ZDD& zdd) const noexcept {
  unsigned char array[36] = {};
  for(int i = 0; i < 36; i++) {
    array[i] = getobjnum(m_array[i]);
  }
  return zdd.compute_id(array, 36);
}

int Posi::getunknowninfo(unsigned char zdd_code[70][36]) const noexcept {
  int nblief = 0;
  int enemy_blue_count = 0, unknown_count = 0;
  
  foreach_comb
    (m_enemy_blue_count + m_enemy_red_count, m_enemy_blue_count, [&](int *indexes)
      {
	enemy_blue_count = 0, unknown_count = 0;
	for(int i = 0; i < 36; i++) {
	  if((m_array[i].piece & PLAYER_MASK) == enemy) {
	    assert(m_array[i].piece == (unknown|enemy));
	    if(enemy_blue_count < m_enemy_blue_count && indexes[enemy_blue_count] == unknown_count) {
	      zdd_code[nblief][i] = 4; // (blue|enemy)
	      enemy_blue_count++;
	      assert(enemy_blue_count <= m_enemy_blue_count);
	    } else {
	      zdd_code[nblief][i] = 5; // (red|enemy)
	    }
	    unknown_count++;
	    assert(unknown_count <= m_enemy_blue_count + m_enemy_red_count);
	  } else {
	    zdd_code[nblief][i] = getobjnum(m_array[i]);
	  }
	}
	nblief++;
	assert(nblief > 0 && nblief <= 70);
      });
  
  return nblief;
}

int Posi::make_action(const Action& action) noexcept {
  Snapshot s;
  // copy the current state
  copy(m_array, m_array + 36, s.m_array);
  s.m_self_blue_count = m_self_blue_count;
  s.m_self_red_count = m_self_red_count;
  s.m_enemy_blue_count = m_enemy_blue_count;
  s.m_enemy_red_count = m_enemy_red_count;
  m_history.push(s);

  // apply the move
  int before = action.get_loc1();
  int after = action.get_loc2();
  assert(before >= 0 && before < 36);

  // if this move reaches the goal
  if(after == 100) {
    int result;
    if(before == 30 || before == 35) result = -1; // win
    else {
      assert(before == 0 || before == 5);
#ifdef USE_PURPLE
      assert(m_array[before].piece == (unknown|enemy));
      result = -2;
#else
      if(m_array[before].piece == (blue|enemy)) result = -2; // lose
      else {
	assert(m_array[before].piece == (red|enemy));
	result = -1; // win(illegal move)
      }
#endif
    }
    m_array[before] = tbl_objid2locinfo[0];
    return result;
  }

  // move a piece
  int result = 0;
  assert(after >= 0 && after < 36);
  if((m_array[after].piece & COLOR_MASK) != empty) {
    char capture = m_array[after].piece;
    if((capture & PLAYER_MASK) == self) {
      if(capture == (blue|self)) {
	m_self_blue_count--;
	result = 1;
      } else {
	assert(capture == (red|self));
	m_self_red_count--;
	result = 2;
      }
    } else {
      assert((capture & PLAYER_MASK) == enemy);
#ifdef USE_PURPLE
      assert(capture == (unknown|enemy));
      m_enemy_red_count--;
      result = 2;
#else
      if(capture == (blue|enemy)) {
	m_enemy_blue_count--;
	result = 1;
      } else {
	assert(capture == (red|enemy));
	m_enemy_red_count--;
	result = 2;
      }
#endif
    }
  }
  m_array[after] = m_array[before];
  m_array[before] = tbl_objid2locinfo[0];

  if(m_enemy_blue_count == 0 || m_self_red_count == 0) return -1; // win
  if(m_self_blue_count == 0 || m_enemy_red_count == 0) return -2; // lose

  return result;
}

void Posi::undo_action() noexcept {
  Snapshot s = m_history.top();
  m_history.pop();

  copy(s.m_array, s.m_array + 36, m_array);
  m_self_blue_count = s.m_self_blue_count;
  m_self_red_count = s.m_self_red_count;
  m_enemy_blue_count = s.m_enemy_blue_count;
  m_enemy_red_count = s.m_enemy_red_count;
}

vector<string> split(const string text, const char delimiter='/');



// fen format
// forced win
// ./gened 6/1b4/UU4/1U4/2r1b1/r5 1 2221

// forced loss
// ./gened 3U2/2b3/2rb2/4r1/2U3/3U2 1 2221

// can lose
// ./gened U5/2b3/6/U1U1b1/rr4/6 1 2221

// no lose
// ./gened 6/r1rU2/6/b1UU2/6/3b2 1 2221
