#ifndef POSI_HPP
#define POSI_HPP
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include "zdd.hpp"

using namespace std;

// constexpr size_t max_table_size = 7539840ULL;  //求める駒割の配置数

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
    signed char m_before;     //移動前のマス
    signed char m_after;      //移動後のマス
public:
    Action() noexcept : m_before(-1) {}
    Action(int before, int after) noexcept : m_before(before), m_after(after) {}
    int get_before() const noexcept{return m_before;}
    int get_after() const noexcept {return m_after;}
};

//配置(posi)のクラス
class POSITION {
private:
    unsigned int m_turn;                          //現在の手番が敵か味方か(味方 : 0, 敵 : 1)
    Combination last_flip;                        //盤面を反転したときの紫駒の組み合わせを保存する
    Action m_last_action_p1;                      // player1の直前の行動を保存する
    unsigned int m_last_capture_p1;               // player1の直前の行動で駒を取ったかどうか(0U : 何も取っていない, 3U : player2の青駒, 5U : player2の赤駒, 7U : player2の紫駒)
    Action m_last_action_p2;                      // player2の直前の行動を保存する
    unsigned int m_last_capture_p2;               // player2の直線の行動で駒を取ったかどうか(0U : 何も取っていない, 2U : player1の青駒, 4U : player1の赤駒)
    unsigned char array_sq[40];                   //現在の配置を保存
    unsigned char num_b, num_r, num_eb, num_er;   //現在の配置の各駒の数(青、赤、敵青、敵赤)
    unsigned long long int now_id; //現在のid
    int iter;
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
    POSITION(int iter, int i, int j, int k, int l, unsigned long long int id) noexcept : iter(iter), m_last_action_p1(Action()), m_last_capture_p1(0),
                                m_last_action_p2(Action()), m_last_capture_p2(0), last_flip(Combination()),
                                num_b(i), num_r(j), num_eb(k), num_er(l), now_id(id) {
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
};
#endif