#ifndef POSI_GEISTER_H
#define POSI_GEISTER_H
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <array>
#include <stack>
#include "zdd_geister.hpp"

using std::array;
using std::stack;
using namespace std;

struct LocInfo {
  uint8_t piece;
  char piece_ch;
  unsigned char nb;
  unsigned char nr;
  unsigned char uk;

  bool equal(const LocInfo &o) const noexcept {
    return (piece == o.piece && piece_ch == o.piece_ch && nb == o.nb && nr == o.nr && uk == o.uk);
  }
};

constexpr LocInfo tbl_objid2locinfo[6] = {
  {(empty|self)   , '.', 0, 0, 0},   //0
  {(unknown|enemy), 'u', 0, 0, 1},   //1
  {(blue|self)    , 'B', 1, 0, 0},   //2
  {(red|self)     , 'R', 0, 1, 0},   //3
  {(blue|enemy)   , 'b', 1, 0, 0},   //4
  {(red|enemy)    , 'r', 0, 1, 0}    //5
};					  

// Class representing a legal action
class Action{
private:
  int m_loc1;   // square index before the move
  int m_loc2;   // square index after the move (100 indicates the goal)
public:
  Action() noexcept : m_loc1(-1), m_loc2(-1){}
  Action(int loc1, int loc2) noexcept : m_loc1(loc1), m_loc2(loc2) {}
  int get_loc1() const noexcept{return m_loc1;}
  int get_loc2() const noexcept {return m_loc2;}
};

// Class representing a position (Posi)
class Posi {
private:
  struct Snapshot {
    LocInfo m_array[36];
    
    int m_self_blue_count;
    int m_self_red_count;
    int m_enemy_blue_count;
    int m_enemy_red_count;
  };

  LocInfo m_array[36];   // store the current configuration
  
  int m_self_blue_count;
  int m_self_red_count;
  int m_enemy_blue_count;
  int m_enemy_red_count;   // number of pieces of each type in the current configuration (blue, red, opponent's blue, opponent's red)
  
  stack<Snapshot> m_history;
  
public:
  Posi() noexcept;
  Posi(unsigned long long int x, const ZDD& zdd,
       int self_blue_count, int self_red_count, int enemy_blue_count, int enemy_red_count) noexcept;
  void make_posi(unsigned long long int x, const ZDD& zdd,
		 int self_blue_count, int self_red_count, int enemy_blue_count, int enemy_red_count) noexcept;
  void make_posi_n(unsigned char zdd_code[70][36], int n) noexcept;
  void make_posi_opponent() noexcept;
  void make_posi_myself() noexcept;
  void print() const noexcept;
  int compute_actions(Action actions[1000], int iter) noexcept;
  int getobjnum(const LocInfo& a) const noexcept;
  unsigned long long int getzddnum(const ZDD& zdd) const noexcept;
  int getunknowninfo(unsigned char zdd_code[70][36]) const noexcept;
  int make_action(const Action& action) noexcept;
  void undo_action() noexcept;
};  

#endif
