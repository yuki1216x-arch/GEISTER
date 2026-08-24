#include "node.hpp"

bool LocInfo_vali::ok() const noexcept {
  // zero piece
  if((m_piece & COLOR_MASK) == empty) {
    if((m_piece & PLAYER_MASK) != self) return false;
    if(m_nb != 0) return false;
    if(m_nr != 0) return false;
    if(m_uk != 0) return false; }
  // one piece
  else if((m_piece & COLOR_MASK) == blue) {
    if(m_nb != 1) return false;
    if(m_nr != 0) return false;
    if(m_uk != 0) return false; }
  else if((m_piece & COLOR_MASK) == red) {
    if(m_nb != 0) return false;
    if(m_nr != 1) return false;
    if(m_uk != 0) return false; }
  else if((m_piece & COLOR_MASK) == unknown) {
    if((m_piece & PLAYER_MASK) != enemy) return false;
    if(m_nb != 0) return false;
    if(m_nr != 0) return false;
    if(m_uk != 1) return false; }

  return true;
}

const LocInfo_vali tbl_objid2locinfo[6] =
  { LocInfo_vali((empty|self),    0, 0, 0),   //0
    LocInfo_vali((unknown|enemy), 0, 0, 1),   //1
    LocInfo_vali((blue|self),     1, 0, 0),   //2
    LocInfo_vali((red|self),      0, 1, 0),   //3
    LocInfo_vali((blue|enemy),    1, 0, 0),   //4
    LocInfo_vali((red|enemy),     0, 1, 0)    //5
  };

Node::Node(Node& cur, int x) noexcept {
  if(cur.m_loc_stateid == 3) {
    m_locid = cur.m_locid + 1, m_loc_stateid = 0;
  } else {
    m_locid = cur.m_locid, m_loc_stateid = cur.m_loc_stateid + 1;
  }

  // f: propagete the parent's state; if x == 1, the parent's object is placed
  if(cur.m_loc_stateid != 3 && (cur.m_f == 1 || x == 1)) m_f = 1;
  else m_f = 0;

  if(x == 1) {
    m_nb = cur.m_nb + tbl_objid2locinfo[cur.m_loc_stateid].get_nb();
    m_nr = cur.m_nr + tbl_objid2locinfo[cur.m_loc_stateid].get_nr();
    m_ne = cur.m_ne + tbl_objid2locinfo[cur.m_loc_stateid].get_uk();
  } else {
    m_nb = cur.m_nb, m_nr = cur.m_nr, m_ne = cur.m_ne;
  }

  m_num = 0, m_visited = false;
  m_lengthmax = 0, m_lengthmin = 0, m_lengthsum = 0;
}

bool Node::IsNextLeaf0(int depth, int x, int board_nb, int board_nr, int board_ne) const noexcept {
  assert(0 <= depth && depth < 144);
  assert(x == 0 || x == 1);
  //assert(1 <= board_nb && board_nb <= 4);
  //assert(1 <= board_nr && board_nr <= 4);
  //assert(2 <= board_ne && board_ne <= 8);

  if(x == 0) {
    if(m_loc_stateid == 3 && m_f == 0) return true; // Invalid: value remains unset
    if(m_loc_stateid == 0 && (36 - m_locid) - (board_nb - m_nb) - (board_nr - m_nr) - (board_ne - m_ne) < 0) return true;
  } else {
    if(m_f == 1) return true; // Invalid: attempting to place an object on an already occupied square
    if(board_ne - m_ne - tbl_objid2locinfo[m_loc_stateid].get_uk() < 0) return true;
    if(board_nb - m_nb - tbl_objid2locinfo[m_loc_stateid].get_nb() < 0) return true;
    if(board_nr - m_nr - tbl_objid2locinfo[m_loc_stateid].get_nr() < 0) return true;

    if(m_loc_stateid == 0 && (36 - (m_locid + 1)) - (board_nb - m_nb) - (board_nr - m_nr) - (board_ne - m_ne) < 0) return true;
  }

  return false;
}
