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

// constexpr size_t max_table_size = 7539840ULL;  //求める駒割の配置数

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

// 配置の組み合わせを表すクラス
class Combination{
private:
    int blue_position[4];   //青駒があるポイント番号
public:
    Combination() noexcept {
        for(int i = 0; i < 4; i++) {
            blue_position[i] = 0;
        }
    }
    Combination(int position[4]) noexcept {
        for(int i = 0; i < 4; i++) {
            blue_position[i] = position[i];
        }
    }
    void get_blue_position(int postion[4]) const noexcept {
        for(int i = 0; i < 4; i++) {
            postion[i] = blue_position[i];
        }
    }
};

//合法手(action)のクラス
class Action{
private:
  int m_loc1;     //移動前のマスid
  int m_loc2;      //移動後のマスid (100ならゴール)
public:
  Action() noexcept : m_loc1(-1), m_loc2(-1){}
  Action(int loc1, int loc2) noexcept : m_loc1(loc1), m_loc2(loc2) {}
  int get_loc1() const noexcept{return m_loc1;}
  int get_loc2() const noexcept {return m_loc2;}
};

//配置(posi)のクラス
class Posi {
private:
  struct Snapshot {
    LocInfo m_array[36];
    
    int m_self_blue_count;
    int m_self_red_count;
    int m_enemy_blue_count;
    int m_enemy_red_count;
  };

  LocInfo m_array[36];   //現在の配置を保存
  
  int m_self_blue_count;
  int m_self_red_count;
  int m_enemy_blue_count;
  int m_enemy_red_count;   //現在の配置の各駒の数(青、赤、敵青、敵赤)
  
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
  

//配置(posi)のクラス
/*class POSITION {
private:
  unsigned int m_turn;                          //現在の手番が敵か味方か(味方 : 0, 敵 : 1)
  Action m_last_action_p1;                      // player1の直前の行動を保存する
  unsigned int m_last_capture_p1;               // player1の直前の行動で駒を取ったかどうか(0U : 何も取っていない, 3U : player2の青駒, 5U : player2の赤駒, 7U : player2の紫駒)
  Action m_last_action_p2;                      // player2の直前の行動を保存する
  unsigned int m_last_capture_p2;               // player2の直線の行動で駒を取ったかどうか(0U : 何も取っていない, 2U : player1の青駒, 4U : player1の赤駒)
  Combination last_flip;                        //盤面を反転したときの紫駒の組み合わせを保存する
  unsigned char num_b, num_r, num_eb, num_er;   //現在の配置の各駒の数(青、赤、敵青、敵赤)
  unsigned long long int now_id; //現在のid
  unsigned char array_sq[40];                   //現在の配置を保存
  int m_iter;
public:
  POSITION() noexcept : m_turn(player1), m_last_action_p1(Action()), m_last_capture_p1(0),
      m_last_action_p2(Action()), m_last_capture_p2(0), last_flip(Combination()),
      num_b(0), num_r(0), num_eb(0), num_er(0), now_id(0) {
    for(int i = 0; i < 40; i++) {
      array_sq[i] = 0U;
    }
  }
  POSITION(int i, int j, int k, int l, unsigned long long int id, unsigned int turn) noexcept : m_turn(turn), m_last_action_p1(Action()), m_last_capture_p1(0),
												m_last_action_p2(Action()), m_last_capture_p2(0), last_flip(Combination()),
												num_b(i), num_r(j), num_eb(k), num_er(l), now_id(id) {
    ZDD::get(num_b, num_r, num_eb, num_er).compute_array(id, array_sq);
    // array_sq[i]=(red|player2); red of player2 exists at square i.
  }
  POSITION(int iter, int i, int j, int k, int l, unsigned long long int id) noexcept : m_last_action_p1(Action()), m_last_capture_p1(0),
										       m_last_action_p2(Action()), m_last_capture_p2(0), last_flip(Combination()),
										       num_b(i), num_r(j), num_eb(k), num_er(l), now_id(id), m_iter(iter) {
    if(iter % 2 == 1) m_turn = player1;
    else if(iter % 2 == 0) m_turn = player2;
    ZDD::get(iter, num_b, num_r, num_eb, num_er).compute_array(id, array_sq);
  }
    ~POSITION() noexcept {}
    unsigned char get_blue() const noexcept {return num_b;}
    unsigned char get_red() const noexcept {return num_r;}
    unsigned char get_enemy_blue() const noexcept {return num_eb;}
    unsigned char get_enemy_red() const noexcept {return num_er;}
    unsigned int get_turn() const noexcept {return m_turn;}
    unsigned int get_last_capture_p1() const noexcept {return m_last_capture_p1;}
    unsigned int get_last_capture_p2() const noexcept {return m_last_capture_p2;}
    int gen_actions(Action actions[1024]) const noexcept;    // 合法手をすべて求める関数
    void do_action(const Action& action) noexcept;           // 行動 action を実行する関数
    void undo_action() noexcept;                             // 手番を１つ戻す関数
    void do_color(unsigned int color) noexcept;              // 公開可能な色を公開する
    void undo_color() noexcept;                              // 公開した色をもとに戻す
    int gen_belief_state(Combination combinations[70]) const noexcept;
    void do_belief_state(const Combination& combination) noexcept;
    void undo_belief_state() noexcept;
    void do_flip() noexcept;   // 手番を player1 から player2 にし、盤面を反転させる関数(通常ルールのガイスターのみで使用する)
    void undo_flip() noexcept;
    void print_board() const noexcept;                       // 現在の配置を出力する関数
    unsigned long long int compute_id() noexcept;
    void print_capture() const noexcept {std::cout << "capture1: " << (int)m_last_capture_p1 << ", capture2: " << (int)m_last_capture_p2 << endl;}
    int is_end() const noexcept;   // 終端節点の評価値を返す関数
    void change_player(unsigned char array_sq[36]) noexcept;
    void fen_to_array(string fen, string turn, int i, int j, int k, int l) noexcept;
    string array_to_fen() const noexcept;
    };*/
#endif
