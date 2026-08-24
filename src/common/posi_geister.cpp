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

// nCkの組み合わせに対して処理を実行する
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
	    if(indexes[enemy_blue_count] == unknown_count) {
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
      if(m_array[before].piece == (blue|enemy)) result = -2; // lose
      else {
	assert(m_array[before].piece == (red|enemy));
	result = -1; // win(illegal move)
      }
    }
    m_array[before] = tbl_objid2locinfo[0];
    return result;
  }

  // move a piece
  int result = 0;
  assert(after >= 0 && after < 36);
  if((m_array[after].piece & COLOR_MASK) != empty) {
    char capture = m_array[after].piece;
    if(capture == (blue|self)) {
      m_self_blue_count--;
      result = 1;
    } else if(capture == (red|self)) {
      m_self_red_count--;
      result = 2;
    } else if(capture == (blue|enemy)) {
      m_enemy_blue_count--;
      result = 1;
    } else {
      assert(capture == (red|enemy));
      m_enemy_red_count--;
      result = 2;
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


//合法手を求める関数 (hand = 0 : 味方手番, hand = 1 : 相手手番)
/*int POSITION::gen_actions(Action actions[1024]) const noexcept {
    if(is_end() != 0) return 0;
    int index = 0;
    if(m_turn == player1) {
        if(array_sq[30] == (blue|player1)) actions[index] = Action(30, 38), index++;
        else if(array_sq[35] == (blue|player1)) actions[index] = Action(35, 39), index++;
        else {
            for(unsigned char num_square = 0; num_square < 36; num_square++) {
                if(array_sq[num_square] == (blue|player1) || array_sq[num_square] == (red|player1)) {
                    //下移動
                    if(6 <= num_square) {
                        if(array_sq[num_square-6] != (blue|player1) && array_sq[num_square-6] != (red|player1)) {
                            actions[index] = Action(num_square, num_square-6);
                            index++;
                        }
                    }
                    //上移動
                    if(num_square <= 29) {
                        if(array_sq[num_square+6] != (blue|player1) && array_sq[num_square+6] != (red|player1)) {
                            actions[index] = Action(num_square, num_square+6);
                            index++;
                        }
                    }
                    //右移動
                    if(num_square % 6 != 0) {
                        if(array_sq[num_square-1] != (blue|player1) && array_sq[num_square-1] != (red|player1)) {
                            actions[index] = Action(num_square, num_square-1);
                            index++;
                        }
                    }
                    //左移動
                    if(num_square % 6 != 5) {
                        if(array_sq[num_square+1] != (blue|player1) && array_sq[num_square+1] != (red|player1)) {
                            actions[index] = Action(num_square, num_square+1);
                            index++;
                        }
                    }
                }
            }
        }
    } else if(m_turn == player2) {
        if (array_sq[0] == (purple|player2) || array_sq[0] == (blue|player2)) actions[index] = Action(0, 36), index++;
        else if(array_sq[5] == (purple|player2) || array_sq[5] == (blue|player2)) actions[index] = Action(5, 37), index++;
        else {
            for(unsigned char num_square = 0; num_square < 36; num_square++) {
                // assert(array_sq[num_square] != (blue|player2) && array_sq[num_square] != (red|player2));
                if(array_sq[num_square] == (purple|player2) || array_sq[num_square] == (blue|player2) || array_sq[num_square] == (red|player2)) {
                    //下移動
                    if(6 <= num_square) {
                        if(array_sq[num_square-6] != (purple|player2) && array_sq[num_square-6] != (blue|player2) && array_sq[num_square-6] != (red|player2)) {
                            actions[index] = Action(num_square, num_square-6);
                            index++;
                        }
                    }
                    //上移動
                    if(num_square <= 29) {
                        if(array_sq[num_square+6] != (purple|player2) && array_sq[num_square+6] != (blue|player2) && array_sq[num_square+6] != (red|player2)) {
                            actions[index] = Action(num_square, num_square+6);
                            index++;
                        }
                    }
                    //右移動
                    if(num_square % 6 != 0) {
                        if(array_sq[num_square-1] != (purple|player2) && array_sq[num_square-1] != (blue|player2) && array_sq[num_square-1] != (red|player2)) {
                            actions[index] = Action(num_square, num_square-1);
                            index++;
                        }
                    }
                    //左移動
                    if(num_square % 6 != 5) {
                        if(array_sq[num_square+1] != (purple|player2) && array_sq[num_square+1] != (blue|player2) && array_sq[num_square+1] != (red|player2)) {
                            actions[index] = Action(num_square, num_square+1);
                            index++;
                        }
                    }
                }
            }
        }
    }
    assert(index < 500);
    return index;
}

//行動 action を１つ実行する関数
void POSITION::do_action(const Action& action) noexcept {
    int after  = action.get_after();
    int before = action.get_before();
    unsigned int capture = 0U;
    assert(0 <= after && after <= 39);
    assert(0 <= before && before < 36);
    if(m_turn == player1) {
        capture = array_sq[after];
        // cout << "now capture : " << capture << endl;
        assert(capture == 0U || capture == (purple|player2) || capture == (blue|player2) || capture == (red|player2));
        if(capture != 0U) {
            if(capture == (purple|player2)){
                #ifdef USE_PURPLE
                    capture = (red|player2);
                    num_er--;
                #endif
            } else if(capture == (blue|player2)) num_eb--;
            else if(capture == (red|player2)) num_er--;
            else assert(false);
        }
        array_sq[after] = array_sq[before];
        array_sq[before] = 0U;
        m_turn = player2;
        assert(m_last_action_p1.get_before() == -1);
        m_last_capture_p1 = capture;
        m_last_action_p1 = action;
    } else {
        capture = array_sq[after];
        // if(capture != 0U && capture != (blue|player1) && capture != (red|player1)) cout << capture << endl;
        assert(capture == 0U || capture == (blue|player1) || capture == (red|player1));
        if(capture == (blue|player1)) {
            num_b--;
        } else if(capture == (red|player1)) {
            num_r--;
        }
        array_sq[after] = array_sq[before];
        array_sq[before] = 0U;
        m_turn = player1;
        assert(m_last_action_p2.get_before() == -1);
        m_last_capture_p2 = capture;
        m_last_action_p2 = action;
    }
}

//手番を１つ戻す関数
void POSITION::undo_action() noexcept {
    int after, before;
    unsigned int capture;
    if(m_turn == player2) {                    // 現在の手番が相手のとき1つ前の行動は味方手番
        after = m_last_action_p1.get_after();
        before = m_last_action_p1.get_before();
        capture = m_last_capture_p1;
        assert(before != -1);
        // if(array_sq[after] != (blue|player1) || array_sq[after] != (red|player1)) {
        //     cout << array_sq[after] << endl;
        //     print_board();
        //     cout << after << ':' << before << endl;
        // }
        assert(array_sq[after] == (blue|player1) || array_sq[after] == (red|player1));
        assert(capture == (purple|player2) || capture == (blue|player2) || capture == (red|player2) || capture == 0U);
        // if(capture == (blue|player2)) {
        //     capture = (purple|player2);
        //     num_eb++;
        // } else if(capture == (red|player2)) {
        //     capture = (purple|player2);
        //     num_er++;
        // }
        array_sq[before] = array_sq[after];
        array_sq[after] = capture;
        // 2025/7/8修正 下2行
        if(capture == (blue|player2)) num_eb++;
        else if(capture == (red|player2)) num_er++;
        m_last_capture_p1 = 0U;
        m_last_action_p1 = Action();
        m_turn = player1;
    } else if(m_turn == player1) {
        after = m_last_action_p2.get_after();
        before = m_last_action_p2.get_before();
        capture = m_last_capture_p2;
        assert(before != -1);
        assert(array_sq[after] == (purple|player2) || array_sq[after] == (blue|player2) || array_sq[after] == (red|player2));
        assert(capture == (blue|player1) || capture == (red|player1) || capture == 0U);
        array_sq[before] = array_sq[after];
        array_sq[after] = capture;
        if(capture == (blue|player1)) {
            num_b++;
        } else if(capture == (red|player1)) {
            num_r++;
        }
        m_last_capture_p2 = 0U;
        m_last_action_p2 = Action();
        m_turn = player2;
    }
}

void POSITION::do_color(unsigned int color) noexcept {
    assert(m_turn == player2);
    assert(m_last_capture_p1 == (purple|player2));
    if(color == blue) {
        num_eb--;
        m_last_capture_p1 = (blue|player2);
    } else if(color == red) {
        num_er--;
        m_last_capture_p1 = (red|player2);
    }
}

void POSITION::undo_color() noexcept {
    assert(m_turn == player2);
    assert(m_last_capture_p1 == (blue|player2) || m_last_capture_p1 == (red|player2));
    if(m_last_capture_p1 == (blue|player2)) num_eb++;
    else if(m_last_capture_p1 == (red|player2)) num_er++;
    m_last_capture_p1 = (m_last_capture_p1 | purple);
}

int POSITION::gen_belief_state(Combination combinations[70]) const noexcept {
    int num_piece, p_count = 0;
    int purple_position[8];  // 不明駒のあるポイント番号
    // bool flag = 0; // 直前に駒取りがあったかどうかのフラグ
    num_piece = num_eb + num_er;
    for(int positionid = 0; positionid < 36; positionid++) {
        if(array_sq[positionid] == (purple|player2)) {
            assert(p_count < num_piece);
            purple_position[p_count] = positionid;
            p_count++;
        }
    }
    // if(m_turn == player1){
    //     if(get_last_capture_p2() == 0U) num_piece = num_b + num_r;
    //     else num_piece = num_b + num_r - 1, flag = 1;
    //     for(int positionid = 0; positionid < 36; positionid++) {
    //         if(array_sq[positionid] == (purple|player2)) {
    //             assert(p_count < num_piece);
    //             purple_position[p_count] = positionid;
    //             p_count++;
    //         }
    //     }
    // } else {
    //     if(get_last_capture_p1() == 0U) num_piece = num_eb + num_er;
    //     else num_piece = num_eb + num_er - 1, flag = 1;
    //     for(int positionid = 0; positionid < 36; positionid++) {
    //         if(array_sq[positionid] == (purple|player2)) {
    //             assert(p_count < num_piece);
    //             purple_position[p_count] = positionid;
    //             p_count++;
    //         }
    //     }
    // }
    assert(p_count == num_piece); // 見つけた不明駒の数が青駒と赤駒の合計と一致するか
    int index = 0;
    // cout << p_count << ':' << num_eb << endl;
    foreach_comb(p_count, num_eb, [&](int *indexes) {
        int blue_array[4] = {-1, -1, -1, -1};
        for(int i = 0; i < num_eb; i++) {
            blue_array[i] = purple_position[indexes[i]];
        }
        combinations[index] = Combination(blue_array);
        index++;
        assert(index < 70);
        assert(index > 0);
    });
    // if(flag) {
    //     foreach_comb(p_count, num_b-1, [&](int *indexes) {
    //         int blue_array[4] = {-1, -1, -1, -1};
    //         for(int i = 0; i < num_b-1; i++) {
    //             blue_array[i] = purple_position[indexes[i]];
    //         }
    //         combinations[index] = Combination(blue_array);
    //         index++;
    //         assert(index < 70);
    //         assert(index > 0);
    //     });
    // }
    return index;
}

void POSITION::do_belief_state(const Combination& combination) noexcept {
    int blue_array[4], blue_count = 0, red_count = 0;
    combination.get_blue_position(blue_array);
    // 今回はこっちでいい
    // cout << blue_array[0] << ':' << blue_array[1] << ':' << blue_array[2] << ':' << blue_array[3] << endl;
    for(int positionid = 0; positionid < 36; positionid++) {
        if(array_sq[positionid] == (purple|player2)) {
            if(blue_array[blue_count] == positionid) {
                assert(blue_count < num_eb);
                array_sq[positionid] = (blue|player2);
                blue_count++;
            } else {
                // if(red_count >= num_er) cout << now_id << ':' << red_count << ':' << num_er << ':' << blue_count << endl;
                assert(red_count < num_er);
                array_sq[positionid] = (red|player2);
                red_count++;
            }
        }
    }
    // if(!(blue_count == num_eb || (blue_count == num_eb-1 && m_last_capture_p1 == (purple|player2)))) cout << blue_count << num_eb << endl;
    assert(blue_count == num_eb || (blue_count == num_eb-1 && m_last_capture_p1 == (purple|player2)));
    assert(red_count == num_er || (red_count == num_er-1 && m_last_capture_p1 == (purple|player2)));
    if(blue_count == num_b-1 && m_last_capture_p1 == (purple|player2)) num_eb--, m_last_capture_p1 = (blue|player2);
    else if(red_count == num_r-1 && m_last_capture_p1 == (purple|player2)) num_er--, m_last_capture_p1 = (red|player2);
    // if(m_turn == player1) {
    //     for(int positionid = 0; positionid < 36; positionid++) {
    //         if(array_sq[positionid] == (purple|player1)) {
    //             if(blue_array[blue_count] == positionid) {
    //                 assert(blue_count < num_b);
    //                 array_sq[positionid] = (blue|player1);
    //                 blue_count++;
    //             } else {
    //                 assert(red_count < num_r);
    //                 array_sq[positionid] = (red|player1);
    //                 red_count++;
    //             }
    //         }
    //     }
    //     assert(blue_count == num_b || (blue_count == num_b-1 && m_last_capture_p2 == (purple|player1)));
    //     assert(red_count == num_r || (red_count == num_r-1 && m_last_capture_p2 == (purple|player1)));
    //     if(blue_count == num_b-1 && m_last_capture_p2 == (purple|player1)) num_b--, m_last_capture_p1 == (blue|player1);
    //     else if(red_count == num_r-1 && m_last_capture_p2 == (purple|player1)) num_r--, m_last_capture_p1 == (red|player1);
    // } else {
    //     for(int positionid = 0; positionid < 36; positionid++) {
    //         if(array_sq[positionid] == (purple|player2)) {
    //             if(blue_array[blue_count] == positionid) {
    //                 assert(blue_count < num_eb);
    //                 array_sq[positionid] = (blue|player2);
    //                 blue_count++;
    //             } else {
    //                 assert(red_count < num_er);
    //                 array_sq[positionid] = (red|player2);
    //                 red_count++;
    //             }
    //         }
    //     }
    //     assert(blue_count == num_eb || (blue_count == num_eb-1 && m_last_capture_p1 == (purple|player2)));
    //     assert(red_count == num_er || (red_count == num_er-1 && m_last_capture_p1 == (purple|player2)));
    //     if(blue_count == num_eb-1 && m_last_capture_p1 == (purple|player2)) num_eb--, m_last_capture_p2 == (blue|player2);
    //     else if(red_count == num_er-1 && m_last_capture_p1 == (purple|player2)) num_er--, m_last_capture_p2 == (red|player2);
    // }
}

void POSITION::undo_belief_state() noexcept {
    // 一旦見えないコマはplayer2で
    int enemy_count = 0;
    for(int i = 0; i < 36; i++) {
        if(array_sq[i] == (red|player2) || array_sq[i] == (blue|player2)) {
            array_sq[i] = (purple|player2);
            enemy_count++;
            assert(enemy_count <= num_eb + num_er);
        }
        assert(enemy_count <= num_eb + num_er);
    }
    // こっちも使うときがあるかもしれない
    // if(m_turn == player1) {
    //     for(int i = 0; i < 36; i++) {
    //         if(array_sq[i] == (red|player1) || array_sq[i] == (blue|player1)) {
    //             array_sq[i] = (purple|player1);
    //             enemy_count++;
    //             assert(enemy_count <= num_b + num_r);
    //         }
    //     }
    //     assert(enemy_count == num_b + num_r);
    // } else {
    //     for(int i = 0; i < 36; i++) {
    //         if(array_sq[i] == (red|player2) || array_sq[i] == (blue|player2)) {
    //             array_sq[i] = (purple|player2);
    //             enemy_count++;
    //             assert(enemy_count <= num_eb + num_er);
    //         }
    //     }
    //     assert(enemy_count == num_eb + num_er);
    // }
}

void POSITION::do_flip() noexcept {                                
    // print_board();
    for(int i = 0; i < 18; i++) swap(array_sq[i], array_sq[35-i]);
    swap(array_sq[36], array_sq[39]);
    swap(array_sq[37], array_sq[38]);
    // unsigned long long int rev_id = ZDD::get(iter, num_b, num_r, num_eb, num_er).compute_id_reverse(array_sq);
    // print_board();
    // ZDD::get(iter, num_b, num_r, num_eb, num_er).compute_array(rev_id, array_sq);
    // print_board();
    change_player(array_sq);
    if(m_turn == player1) m_turn = player2;
    else m_turn = player1;
}

void POSITION::undo_flip() noexcept {
    change_player(array_sq);
    for(int i = 0; i < 18; i++) swap(array_sq[i], array_sq[35-i]);
    swap(array_sq[36], array_sq[39]);
    swap(array_sq[37], array_sq[38]);
    // unsigned long long int rev_id = ZDD::get(iter, num_b, num_r, num_eb, num_er).compute_id_reverse(array_sq);
    // ZDD::get(iter, num_b, num_r, num_eb, num_er).compute_array(rev_id, array_sq);
    if(m_turn == player1) m_turn = player2;
    else m_turn = player1;
}

void pbre(unsigned char sq) {
    if(sq == (purple|player2)) {
        cout << "▼";
    } else if(sq == (blue|player2)) {
        cout << "▼";
    } else if(sq == (red|player2)) {
        cout << "▽";
    } else if(sq == (purple|player1)) {
        cout << "▲";
    } else if(sq == (blue|player1)) {
        cout << "▲";
    } else if(sq == (red|player1)) {
        cout << "△";
    } else {
        cout << "丶";
    }
}

// 現在の配置を出力する関数
void POSITION::print_board() const noexcept {
    if(m_turn == player1) cout << "player1" << endl;
    else if(m_turn == player2) cout << "player2" << endl;
    cout << "自分 青 : " << (int)num_b << ", 赤 : " << (int)num_r << endl;
    cout << "相手 青 : " << (int)num_eb << ", 赤 : " << (int)num_er << endl;
    cout << "ゴール1: ";
    pbre(array_sq[39]);
    cout << ", ゴール2: ";
    pbre(array_sq[38]);
    cout << endl;
    cout << "   6 5 4 3 2 1" << endl;
    cout << "  |―――――――――――|" << endl;
    for(int p = 6; p > 0; p--) {
        cout << p << " |";
        for(int q = 1; q <= 6; q++) {
            pbre(array_sq[6*p-q]);
            // if(q != 6) cout << " ";
        }
        cout << "|" << endl;
    }
    cout << "  |―――――――――――|" << endl;
    cout << "ゴール3: ";
    pbre(array_sq[37]);
    cout << ", ゴール4: ";
    pbre(array_sq[36]);
    cout << endl;
    // pbre(array_sq[36]), pbre(array_sq[37]), pbre(array_sq[38]), pbre(array_sq[39]);
}

unsigned long long int POSITION::compute_id() noexcept {
    assert(is_end() == 0);
    return ZDD::get(m_iter, num_b, num_r, num_eb, num_er).compute_id(array_sq);  // 現在の配置のidをを出力する
    // int num_end = is_end();
    // unsigned long long int max_num;
    // if(num_b < 1 || num_r < 1 || num_eb < 1 || num_er < 1) max_num = 0ULL;
    // else                                    max_num = ZDD::get(num_b, num_r, num_eb, num_er).get_num();
    // if(num_end == 0) return max_num+1ULL;
    // else if(num_end == -1) return max_num+3ULL;
    // else if(num_end == -2) return max_num+2ULL;
    // else {
    //     // cout << "num_b : " << (int)num_b << ", num_r : " << (int)num_r << ", num_e : " << num_eb+num_er << endl;
    //     return ZDD::get(num_b, num_r, num_eb, num_er).compute_id(array_sq);              // 現在の配置のidを出力する関数
    // }
}

// 現在の配置の勝敗判定
int POSITION::is_end() const noexcept {
    if(m_turn == player1) {
        if(array_sq[36] == (purple|player2) || array_sq[37] == (purple|player2)) {
            #ifdef USE_PURPLE
                return 2;
            #else
                return 3;
            #endif
        }
        if(array_sq[36] == (blue|player2) || array_sq[37] == (blue|player2)) return 2;
        if(array_sq[38] == (blue|player1) || array_sq[39] == (blue|player1)) return 1;
        if(num_r == 0) return 1;
        if(num_b == 0) return 2;
    } else if(m_turn == player2) {
        if(array_sq[36] == (purple|player2) || array_sq[37] == (purple|player2)) {
            #ifdef USE_PURPLE
                return 2;
            #else
                return 3;
            #endif
        }
        if(array_sq[36] == (blue|player2) || array_sq[37] == (blue|player2)) return 2;
        if(array_sq[38] == (blue|player1) || array_sq[39] == (blue|player1)) return 1;
        if(num_eb == 0) return 1;
        if(num_er == 0) return 2;
    }
    return 0;
}

void POSITION::change_player(unsigned char array_sq[36]) noexcept {
    for(int i = 0; i < 40; i++) {
        if((array_sq[i] & 1U) == player1 && array_sq[i] != 0U) array_sq[i] = (array_sq[i] | player2);
        else if((array_sq[i] & 1U) == player2) array_sq[i] = (array_sq[i] & 14U);
    }
    swap(num_b, num_eb), swap(num_r, num_er);
}

unsigned int make_array(char piece) {
    if(piece == 'U' || piece == 'P') return (purple|player2);
    else if(piece == 'b') return (blue|player1);
    else if(piece == 'r') return (red|player1);
    else if(piece == 'B') return (blue|player2);
    else if(piece == 'R') return (red|player2);
    else if(piece == '1' || piece == '2' || piece == '3' || piece == '4' || piece == '5' || piece == '6') return 0U;
    else terminate();
}

void POSITION::fen_to_array(string fen, string turn, int i, int j, int k ,int l) noexcept {
    // 現在の手番はどちらか
    if(turn == "1") m_turn = player1;
    else if(turn == "2") m_turn = player2;
    else terminate();

    // 残りコマの判別
    num_b = i, num_r = j, num_eb = k, num_er = l;

    // fen形式の判別
    vector<string> cols = split(fen);
    int count_b = 0, count_r = 0, count_e = 0;
    size_t point = 36;
    for (size_t i = 0; i < cols.size(); i++) {
        for (size_t j = 0; j < cols[i].size(); j++) {
            point--;
            array_sq[point] = make_array(cols[i][j]);
            if(array_sq[point] == 0U && cols[i][j] != '1') {
                for(int k=cols[i][j] - '1'; k>0 ; k--) {
                    point--;
                    array_sq[point] = 0U;
                }
            } else if(array_sq[point] != 0U) {
                if(array_sq[point] == (purple|player2) || array_sq[point] == (blue|player2) || array_sq[point] == (red|player2)) count_e++;
                else if(array_sq[point] == (blue|player1)) count_b++;
                else if(array_sq[point] == (red|player1)) count_r++;
            }
	    assert(point >= 0);
        }
        // cout << i << " : " << point << endl;
        assert(point == 36 - (i + 1) * 6);
    }
    assert(count_b == num_b);
    assert(count_r == num_r);
    assert(count_e == num_eb + num_er);    
}

vector<string> split(const string text, const char delimiter) {
    vector<string> columns;

    if (text.empty()) {
        return columns;
    }

    stringstream stream{text};
    string buff;
    while (getline(stream, buff, delimiter)) {
        columns.push_back(buff);
    }
    return columns;
}

string make_fen(unsigned int piece) {
    if(piece == (blue|player1)) return "b";
    else if(piece == (red|player1)) return "r";
    else if(piece == (blue|player2)) return "B";
    else if(piece == (red|player2)) return "R";
    else if(piece == (purple|player1)) return "u";
    else if(piece == (purple|player2)) return "U";
    else {
        cout << piece << endl;
        return "e";
    }
}

string POSITION::array_to_fen() const noexcept {
    // 配列からfen形式へ
    string fen_format = "";
    for(int i = 5; i >= 0; i--) {
        int blank_count = 0;
        for(int j = 5; j >= 0; j--) {
            if(array_sq[i*6 + j] == 0U) blank_count++;
            else {
                if(blank_count != 0) {
                    fen_format += to_string(blank_count);
                    blank_count = 0;
                }
                // cout << i*6 + j << ":  ";
                fen_format += make_fen(array_sq[i*6 + j]);
            }
        }
        if(blank_count != 0) fen_format += to_string(blank_count);
        if(i != 0) fen_format += '/';
    }

    // 手番プレイヤ
    string turn_player;
    if(m_turn == player1) turn_player = '1';
    else if(m_turn == player2) turn_player = '2';

    // 残りコマ数
    string remaining_num = to_string(num_b) + to_string(num_r) + to_string(num_eb) + to_string(num_er);

    // 全部まとめる
    string result = fen_format + ' ' + turn_player + ' ' + remaining_num;
    return result;
    }*/


// fen形式
// 必勝！！
// ./gened 6/1b4/UU4/1U4/2r1b1/r5 1 2221

// 必敗！！
// ./gened 3U2/2b3/2rb2/4r1/2U3/3U2 1 2221

// 負け有り！！
// ./gened U5/2b3/6/U1U1b1/rr4/6 1 2221

// 負け無し！！
// ./gened 6/r1rU2/6/b1UU2/6/3b2 1 2221
