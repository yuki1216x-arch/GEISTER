// 必勝を求める
#include <deque>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <thread>
#include <map>
#include <stdint.h>
#include <climits>
#include <exception>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include "../common/table.hpp"
#include "../common/posi_geister.hpp"
#include "../common/zdd_geister.hpp"

using std::ref;
using std::cref;

using namespace std;

// int NUM_B, NUM_R, NUM_EB, NUM_ER;

constexpr int nworker = 7;  //並列数
constexpr int deq_input_size = 2048;    
constexpr int deq_output_size = 256;
constexpr int max_legal_num = 32;
constexpr int max_belief_state = 70;

condition_variable cv_boss;     //condition_variableもポジックススレッドの排他制御の一つ
condition_variable cv_worker;   
mutex mtx;  
//ポジックススレッディング
//ポジックス-インターフェースの名前
//ここではポジックススレッドのmutexを使ってる

bool flag_worker_quit; //仕事が終わったことを表すフラグ
int flag = 0;
unsigned long long int search_id = 0ULL;

// unsigned long long int keiro[422], length = 0/*, x*/; //-----------------------------

// enum {b000 = 0, b001, b010, b011, b100, b101, b110, b111 };

//仕事を表すクラス
class Work {
private:
  unsigned long long int m_id;    //割り振ったid
  unsigned int m_value;   // bofore value
  int m_nchild;   //m_idの子節点の数
  int m_num_of_un;
  int m_captured_piece_type[max_legal_num][max_belief_state];
  long long int m_array_id[max_legal_num][max_belief_state]; //子節点の値
  int m_captured_piece_type_opp[max_belief_state][max_legal_num];
  long long int m_array_id_opp[max_belief_state][max_legal_num];
  long long int m_array_id_opp_e[max_belief_state];

public:
  Work() noexcept {}
  Work(unsigned long long int id) noexcept : m_id(id) {}
  //void set_num_child(int num_child) noexcept { m_num_child = num_child; } // delete
  void set_id(unsigned long long int id, unsigned int value) noexcept {
    m_id = id;
    m_value = value;
  }
  void set(int nchild, int num_of_un, int captured_piece_type[max_legal_num][max_belief_state], long long int array_id[max_legal_num][max_belief_state]) noexcept {
    assert(nchild > 0 && nchild <= max_legal_num);
    assert(num_of_un > 0 && num_of_un <= max_belief_state);
    m_nchild = nchild;
    m_num_of_un = num_of_un;
    for(int i = 0; i < m_nchild; i++) {
      for(int j = 0; j < m_num_of_un; j++) {
	m_captured_piece_type[i][j] = captured_piece_type[i][j];
	m_array_id[i][j] = array_id[i][j];
      }
    }
  }
  void set_opp(int num_of_un, long long int array_id_opp_e[max_belief_state]) noexcept {
    assert(num_of_un > 0 && num_of_un <= max_belief_state);
    m_num_of_un = num_of_un;
    for(int i = 0; i < m_num_of_un; i++) {
      m_array_id_opp_e[i] = array_id_opp_e[i];
    }
  }
  void set_opp(int nchild, int num_of_un, int captured_piece_type_opp[max_belief_state][max_legal_num], long long int array_id_opp[max_belief_state][max_legal_num]) noexcept {
    assert(nchild > 0 && nchild <= max_legal_num);
    assert(num_of_un > 0 && num_of_un <= max_belief_state);
    m_nchild = nchild;
    m_num_of_un = num_of_un;
    for(int i = 0; i < m_num_of_un; i++) {
      for(int j = 0; j < m_nchild; j++) {
	m_captured_piece_type_opp[i][j] = captured_piece_type_opp[i][j];
	m_array_id_opp[i][j] = array_id_opp[i][j];
      }
    }
  }
  unsigned long long int get_id() const noexcept { return m_id; }
  unsigned int get_value() const noexcept { return m_value; }
  void get(int& nchild, int& num_of_un, int captured_piece_type[max_legal_num][max_belief_state], long long int array_id[max_legal_num][max_belief_state]) const noexcept {
    assert(m_nchild > 0 && m_nchild <= max_legal_num);
    assert(m_num_of_un > 0 && m_num_of_un <= max_belief_state);
    nchild = m_nchild;
    num_of_un = m_num_of_un;
    for(int i = 0; i < m_nchild; i++) {
      for(int j = 0; j < m_num_of_un; j++) {
	captured_piece_type[i][j] = m_captured_piece_type[i][j];
	array_id[i][j] = m_array_id[i][j];
      }
    }
  }
  void get_opp(int& num_of_un, long long int array_id_opp_e[max_belief_state]) const noexcept {
    assert(m_num_of_un > 0 && m_num_of_un <= max_belief_state);
    num_of_un = m_num_of_un;
    for(int i = 0; i < m_num_of_un; i++) {
      array_id_opp_e[i] = m_array_id_opp_e[i];
    }
  }
  void get_opp(int& nchild, int& num_of_un, int captured_piece_type_opp[max_belief_state][max_legal_num], long long int array_id_opp[max_belief_state][max_legal_num]) const noexcept {
    assert(m_nchild > 0 && m_nchild <= max_legal_num);
    assert(m_num_of_un > 0 && m_num_of_un <= max_belief_state);
    nchild = m_nchild;
    num_of_un = m_num_of_un;
    for(int i = 0; i < m_num_of_un; i++) {
      for(int j = 0; j < m_nchild; j++) {
	captured_piece_type_opp[i][j] = m_captured_piece_type_opp[i][j];
	array_id_opp[i][j] = m_array_id_opp[i][j];
      }
    }
  }
};

constexpr unsigned long long int placement_count[5][5][9] {
  { {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL} }
  , { {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 706860ULL, 7539840ULL, 58433760ULL, 350602560ULL, 1694579040ULL, 6778316160ULL, 22876817040ULL}
    , {0ULL, 0ULL, 11309760ULL, 116867520ULL, 876506400ULL, 5083737120ULL, 23724106560ULL, 91507268160ULL, 297398621520ULL}
    , {0ULL, 0ULL, 116867520ULL, 1168675200ULL, 8472895200ULL, 47448213120ULL, 213516959040ULL, 793062990720ULL, 2478321846000ULL}
    , {0ULL, 0ULL, 876506400ULL, 8472895200ULL, 59310266400ULL, 320275438560ULL, 1387860233760ULL, 4956643692000ULL, 14869931076000ULL} }
  , { {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 11309760ULL, 116867520ULL, 876506400ULL, 5083737120ULL, 23724106560ULL, 91507268160ULL, 297398621520ULL}
    , {0ULL, 0ULL, 175301280ULL, 1753012800ULL, 12709342800ULL, 71172319680ULL, 320275438560ULL, 1189594486080ULL, 3717482769000ULL}
    , {0ULL, 0ULL, 1753012800ULL, 16945790400ULL, 118620532800ULL, 640550877120ULL, 2775720467520ULL, 9913287384000ULL, 29739862152000ULL}
    , {0ULL, 0ULL, 12709342800ULL, 118620532800ULL, 800688596400ULL, 4163580701280ULL, 17348252922000ULL, 59479724304000ULL, 171004207374000ULL} }
  , { {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 116867520ULL, 1168675200ULL, 8472895200ULL, 47448213120ULL, 213516959040ULL, 793062990720ULL, 2478321846000ULL}
    , {0ULL, 0ULL, 1753012800ULL, 16945790400ULL, 118620532800ULL, 640550877120ULL, 2775720467520ULL, 9913287384000ULL, 29739862152000ULL}
    , {0ULL, 0ULL, 16945790400ULL, 158160710400ULL, 1067584795200ULL, 5551440935040ULL, 23131003896000ULL, 79306299072000ULL, 228005609832000ULL}
    , {0ULL, 0ULL, 118620532800ULL, 1067584795200ULL, 6939301168800ULL, 34696505844000ULL, 138786023376000ULL, 456011219664000ULL, 1254030854076000ULL} }
  , { {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}
    , {0ULL, 0ULL, 876506400ULL, 8472895200ULL, 59310266400ULL, 320275438560ULL, 1387860233760ULL, 4956643692000ULL, 14869931076000ULL}
    , {0ULL, 0ULL, 12709342800ULL, 118620532800ULL, 800688596400ULL, 4163580701280ULL, 17348252922000ULL, 59479724304000ULL, 171004207374000ULL}
    , {0ULL, 0ULL, 118620532800ULL, 1067584795200ULL, 6939301168800ULL, 34696505844000ULL, 138786023376000ULL, 456011219664000ULL, 1254030854076000ULL}
    , {0ULL, 0ULL, 800688596400ULL, 6939301168800ULL, 43370632305000ULL, 208179035064000ULL, 798019634412000ULL, 2508061708152000ULL, 6583661983899000ULL} }
};

const string base[2] = {"self_table", "enemy_table"};
	  
unsigned long long int nwin = 0, nlose = 0, ncan_lose = 0; // lunknownが最後のunknownの番号(-1しておく)

unsigned long long int count_changes = 0ULL;   //更新があった回数(0になるまで探索を繰り返す)

//紙()
deque<Work *> deq_input;    //ボスが仕事を格納していき、workerがここから取り出して合法手を求める
deque<Work *> deq_output;   //workerが求めたidを格納して、ボスが取り出して表に書き込んでいく

//ボスの方
static void boss(int iter, int num_b, int num_r, int num_eb, int num_er,
		 Table& parent_table, const char* write_filename,
		 const Table& child_table, const Table& child_table_cap_b, const Table& child_table_cap_r,
		 const Table& child_table_opp) noexcept {
  std::cout << "boss" << endl;

  nwin = 0, nlose = 0, ncan_lose = 0;
  count_changes = 0ULL;
  
  unsigned long long int max_placement = placement_count[num_b][num_r][num_eb + num_er];
  unsigned long long int count_input = 0ULL;  //前から見ていっている配置の番号
  unsigned long long int count_output = 0ULL; //仕事の数、これが最後まで行ったら終了(多分)
  
  int nstack_work_idle = deq_input_size + deq_output_size + nworker;  //今動いているworkの数
  Work* stack_work_idle[nstack_work_idle];    //Workの配列(workの棚のようなもの)
  for(int i = 0; i < nstack_work_idle; i++) stack_work_idle[i] = new Work;    //各棚に紙を置いとく
  
  // cout << "zzzz" << endl;
  
  //手数を記録する表を開いておく
  //ofstream os(write_filename, ios::binary | ios::in | ios::ate);
  
  while(true) {// 仕事がなくなるまで繰り返す
    unique_lock<mutex> lck(mtx); // ロック, unique_lockのインスタンスのlckが破棄されると自動的にmtxがアンロック状態になる
    cv_boss.wait(lck, [&](){     //wait(unique_lockのインスタンス, 何かしらの関数(参照,ポインタ,ラムダ式でも可)), ここだとラムダ式の参照渡し
			return (((deq_input.size() < deq_input_size) && (deq_output.size() < deq_output_size) && (count_input < max_placement))
				|| (0 < deq_output.size())); });
    // deq_inputやdeq_outputが十分減るまで待つ(真だったら寝ないし、ロックも解放しない)
    // waitは条件を満たすまで寝る(待つ)(偽の間はずっと待ち状態)。 条件を満たすとwaitがcall_backする。
    // 条件:第二引数の関数が真偽。関数の実行はどのタイミングでもされうる(基本的にはない)。
    // waitが寝ている間は、第一引数(lck)がアンロック状態になる。       
    // ロックの解放と寝るのはアトミック(同時)
    // 起きたなら、ロックは獲得している。起きてから式の評価を1回行い、偽ならばロックを解放して再び寝る。真ならば、コールバック。
    // deq_input.size(), deq_output.size()が小さいならunknownのidを見つけて、そのworkを作る(①の処理)
    if ((deq_input.size() < deq_input_size) && (deq_output.size() < deq_output_size) && count_input < max_placement) {
      // 後退解析の表を使う。
      // add only unknown id
      unsigned int before_value;
      while(count_input < max_placement && (before_value = parent_table.get(count_input)) != v_unknown && before_value != v_can_lose) {
	count_input++;      //ここで今求めたunknownのidの値が入る
	count_output++;     //ここで今求めたunknownのidの値が入る
      }

      /*while(count_input < max_placement && (before_value = parent_table.get(count_input)) != v_unknown) {    //unknownが見つかるまで
	count_input++;      //ここで今求めたunknownのidの値が入る
	count_output++;     //ここで今求めたunknownのidの値が入る
      }*/
      if(count_input >= max_placement) {
	lck.unlock();
	continue;
      }

      // if(count_input % 1000000000ULL == 0ULL) std::cout << "count_input: " << count_input << ", nwin: " << nwin << ", nexist_lose: " << ncan_lose << ", nunknown: " << nunknown << endl;
                
      assert(nstack_work_idle >= 1);
      Work *pw = stack_work_idle[ --nstack_work_idle ];   //割り当てるworkをstack_work_idleから持ってくる
      pw->set_id(count_input, before_value);    //workerに渡す配置番号を決定
      deq_input.push_front(pw);   //新たに仕事を追加する
      lck.unlock();
      count_input++;              //次のidは今のcount_inputの次の値なのでインクリメント
      
      if(count_input % 1000000000ULL == 0ULL) std::cout << "count_input: " << count_input << ", nwin: " << nwin << ", ncan_lose: " << ncan_lose << ", nlose: " << nlose << endl;
      
      cv_worker.notify_one(); //他スレッドを起こす
    } else {    //仕事がたまって来た場合(③の処理)
      if (0 < deq_output.size()) {
	assert(0 < deq_output.size());
	deque<Work *> deq_tmp; // use swap
	swap(deq_tmp, deq_output);
	
	// empty deq_output
	deq_output.clear();
	lck.unlock();   // 早くunlockするために他のスレッドが触らないdeq_tmpにswapしている
	count_output += deq_tmp.size();
	for (unsigned int workid = 0; workid < deq_tmp.size(); workid++) {
	  unsigned long long int id = deq_tmp[workid]->get_id();  //workに割り当てられている配置番号を見る
	  unsigned int value = deq_tmp[workid]->get_value();
	  
	  if(value == v_unknown) {
	    int nchild, num_of_un;
	    int captured_piece_type[max_legal_num][max_belief_state];
	    long long int array_id[max_legal_num][max_belief_state];
	    deq_tmp[workid]->get(nchild, num_of_un, captured_piece_type, array_id);          //そのidの子供の数(合法手の数)を得る
	    bool already_decided = false;
	    int num_of_action = nchild;
	    
	    assert(nchild > 0);   //子供がいないとおかしい
	    int win_plan_num = 0;  // number of forced-win strategies
	    int can_lose_plan_num = 0;  // number of exist-loss strategies
	    for(int j = 0; j < num_of_action; j++) {
	      int kati = 0;
	      int make = 0;
	      int makeari = 0;
	      int humei = 0;
	      for(int k = 0; k < num_of_un; k++) {
		long long int num_of_haiti = array_id[j][k];
		if(num_of_haiti >= 0) {   // not terminal; retrieve the value from the database
		  int child_val;
		  int which_table = captured_piece_type[j][k];
		  // no piece captured
		  if(which_table == 0) child_val = child_table.get((unsigned long long int)num_of_haiti);
		  // blue piece captured
		  else if(which_table == 1) child_val = child_table_cap_b.get((unsigned long long int)num_of_haiti);
		  // red piece captured
		  else {
		    assert(which_table == 2);
		    child_val = child_table_cap_r.get((unsigned long long int)num_of_haiti);
		  }
		  
		  if(child_val == v_win) kati++;
		  else if(child_val == v_lose) make++;
		  else if(child_val == v_can_lose) makeari++;
		  else {
		    assert(child_val == v_unknown);
		    humei++;
		  }
		} else {
		  if(num_of_haiti == -1) {
		    // player1 wins
		    kati++;
		  } else if(num_of_haiti == -2) {
		    // player2 loses
		    make++;
		  } else {
		    // Do not count this move because it is illegal
		  }
		}
	      }
	      
	      if(kati == num_of_un) { // forced win
		if(iter % 2 == 1) {
		  if(!already_decided) {
		    parent_table.set(id, v_win);
		    nwin++, count_changes++;
		    already_decided = true;
		    value = v_win;
		  }
		} else {
		  win_plan_num++;
		}
	      } else if(make + makeari >= 1) { // can lose
		if(iter % 2 == 1) {
		  can_lose_plan_num++;
		} else {
		  if(!already_decided) {
		    parent_table.set(id, v_can_lose);
		    value = v_can_lose;
		    ncan_lose++;
		    count_changes++;
		    already_decided = true;
		  }
		}
	      }
	    }
	  
	    // after checking all legal moves
	    if(!already_decided) {
	      if(iter % 2 == 1) {
		if(can_lose_plan_num == num_of_action) {  // can lose
		  parent_table.set(id, v_can_lose);
		  value = v_can_lose;
		  ncan_lose++;
		  count_changes++;
		}
	      } else {
		if(win_plan_num == num_of_action) { // forced win
		  parent_table.set(id, v_win);
		  nwin++, count_changes++;
		  value = v_win;
		}
	      }
	    }
	  }

	  if(value == v_unknown || value == v_can_lose) {
	    if(iter % 2 == 1) {
	      int nchild, num_of_un;
	      int captured_piece_type_opp[max_belief_state][max_legal_num];
	      long long int array_id_opp[max_belief_state][max_legal_num];
	      deq_tmp[workid]->get_opp(nchild, num_of_un, captured_piece_type_opp, array_id_opp);
	      int num_of_action = nchild;
	      bool already_decided = false;
	      int lose_plan_num = 0;  // number of loss strategies
	      assert(nchild > 0);   //子供がいないとおかしい
	      for(int j = 0; j < num_of_un; j++) {
		int kati = 0;
		int make = 0;
		int makeari = 0;
		int humei = 0;
		for(int k = 0; k < num_of_action; k++) {
		  long long int num_of_haiti = array_id_opp[j][k];
		
		  if(num_of_haiti >= 0) {   // not terminal; retrieve the value from the database
		    int child_val;
		    int which_table = captured_piece_type_opp[j][k];
		    // no piece captured
		    if(which_table == 0) child_val = child_table.get((unsigned long long int)num_of_haiti);
		    // blue piece captured
		    else if(which_table == 1) child_val = child_table_cap_b.get((unsigned long long int)num_of_haiti);
		    // red piece captured
		    else {
		      assert(which_table == 2);
		      child_val = child_table_cap_r.get((unsigned long long int)num_of_haiti);
		    }
		    
		    if(child_val == v_win) kati++;
		    else if(child_val == v_lose) make++;
		    else if(child_val == v_can_lose) makeari++;
		    else {
		      assert(child_val == v_unknown);
		      humei++;
		    }
		  } else {
		    if(num_of_haiti == -1) {
		      // player1 wins
		      kati++;
		    } else if(num_of_haiti == -2) {
		      // player2 loses
		      make++;
		    } else {
		      // Do not count this move because it is illegal
		    }
		  }
		}
	      
		if(make == num_of_action) { // forced loss
		  lose_plan_num++;
		}
	      }
	  
	      // after checking all legal moves
	      if(!already_decided) {
		if(lose_plan_num == num_of_un) { // forced loss
		  parent_table.set(id, v_lose);
		  nlose++, count_changes++;
		  value = v_lose;
		}
	      }
	    } else {
	      assert(iter % 2 == 0);
	      int num_of_un;
	      long long int array_id_opp_e[max_belief_state];
	      deq_tmp[workid]->get_opp(num_of_un, array_id_opp_e);
	      int kati = 0;
	      int make = 0;
	      int makeari = 0;
	      int humei = 0;
	      for(int j = 0; j < num_of_un; j++) {
		long long int num_of_haiti = array_id_opp_e[j];
		assert(num_of_haiti >= 0);
		int child_val = child_table_opp.get((unsigned long long int)num_of_haiti);
		    
		if(child_val == v_win) kati++;
		else if(child_val == v_lose) make++;
		else if(child_val == v_can_lose) makeari++;
		else {
		  assert(child_val == v_unknown);
		  humei++;
		}
	      }

	      if(kati == num_of_un) {
		parent_table.set(id, v_lose);
		nlose++, count_changes++;
		value = v_lose;
	      }
	    }
	  }
	}
	  
	// return the paper to the shelf
	for(size_t i = 0; i < deq_tmp.size(); i++){
	  assert(nstack_work_idle < 4096);
	  stack_work_idle[ nstack_work_idle++ ] = deq_tmp[i];
	}
      }
      
      // stop if the number of processed configurations exceeds the total number of configuration
      if (count_output >= max_placement) break;
    }
  } // when this scope ends, lck is destroyed and mtx is unlocked
  
  cout << "before_outtable" << endl;
  {
    OutTable out_table(iter, write_filename, max_placement, 2);
    for (unsigned long long int i = 0; i < max_placement; i++) {
      out_table.write(parent_table.get(i));   // retrieve the value of id(i) and copy it directly
    }
    out_table.flush();
    out_table.outinfo();
  }
  
  {
    unique_lock<mutex> lck(mtx); // lock
    flag_worker_quit = true; // flag indicating that the work is finished
  }
  cv_worker.notify_all();
}
  
static void worker(int iter, int num_b, int num_r, int num_eb, int num_er,
		   const ZDD& zdd_parent, const ZDD& zdd_child_cap_b, const ZDD& zdd_child_cap_r,
		   const ZDD& zdd_parent_opp) noexcept {
  Work *w;
  while (true) { //仕事を全て終えるまで繰り返す
    unique_lock<mutex> lck(mtx); //ロック
    //ロック解除して寝る
    cv_worker.wait(lck, [&](){ if (0 < deq_input.size()) return true;
	return flag_worker_quit; }); //仕事がある or (表に記入すべきものがあり、表がkeepされていない) or 仕事が全て終わっている になるまで待つ
    //ロック
    if (flag_worker_quit) break;//仕事が全て完了したなら終わり
    assert(0 < deq_input.size()); //仕事があるなら
    w = deq_input.back();   //deq_inputから取ってくる(コピー)
    deq_input.pop_back();   //取ったやつを消す
    lck.unlock();//ロック解除(deq_inputを同時に触らないようにするためのロック)
    
    // 整数値から、子供の整数値列挙 or ダイレクト勝ちありを求めて w に登録
    unsigned long long int id = w->get_id();
    unsigned int value = w->get_value();
    if(value == v_unknown) {
      Posi p;
      p.make_posi(id, zdd_parent, num_b, num_r, num_eb, num_er);     //wのidのposiを作る
      Action actions[max_legal_num];
      unsigned char board_belief[70][36];
      int nchild = p.compute_actions(actions, iter); // posiの合法手列挙
      int num_of_un = p.getunknowninfo(board_belief);
      assert(nchild > 0 && nchild < max_legal_num);
      assert(num_of_un > 0 && num_of_un <= max_belief_state);
      
      int captured_piece_type[max_legal_num][max_belief_state] = {};
      long long int array_id[max_legal_num][max_belief_state];
      
      for(int i = 0; i < nchild; i++) { //子供ごとの実行
	for(int j = 0; j < num_of_un; j++) {
	  p.make_posi_n(board_belief, j); //
	  int board_check = p.make_action(actions[i]);
	  if(board_check >= 0) { //
	    p.make_posi_myself();
	    if(board_check == 0) {
	      array_id[i][j] = p.getzddnum(zdd_parent);
	    } else if(board_check == 1) {
	      array_id[i][j] = p.getzddnum(zdd_child_cap_b);
	      captured_piece_type[i][j] = 1;
	    } else {
	      assert(board_check == 2);
	      array_id[i][j] = p.getzddnum(zdd_child_cap_r);
	      captured_piece_type[i][j] = 2;
	    }
	  } else {
	    array_id[i][j] = board_check;
	  }
	  p.undo_action();
	}
      }
      w->set(nchild, num_of_un, captured_piece_type, array_id);    //子供の数と孫の数とその配置の値の配列をセット
    }

    if(value == v_unknown || value == v_can_lose) {
      Posi p;
      p.make_posi(id, zdd_parent, num_b, num_r, num_eb, num_er);
      Action actions[max_legal_num];
      unsigned char board_belief[70][36];
      int nchild = p.compute_actions(actions, iter); // posiの合法手列挙
      int num_of_un = p.getunknowninfo(board_belief);
      assert(nchild > 0 && nchild < max_legal_num);
      assert(num_of_un > 0 && num_of_un <= max_belief_state);

      if(iter % 2 == 1) {
	int captured_piece_type_opp[max_belief_state][max_legal_num] = {};
	long long int array_id_opp[max_belief_state][max_legal_num];
      
	for(int i = 0; i < num_of_un; i++) { //子供ごとの実行
	  for(int j = 0; j < nchild; j++) {
	    p.make_posi_n(board_belief, i); //
	    int board_check = p.make_action(actions[j]);
	    if(board_check >= 0) { //
	      p.make_posi_myself();
	      if(board_check == 0) {
		array_id_opp[i][j] = p.getzddnum(zdd_parent);
	      } else if(board_check == 1) {
		array_id_opp[i][j] = p.getzddnum(zdd_child_cap_b);
		captured_piece_type_opp[i][j] = 1;
	      } else {
		assert(board_check == 2);
		array_id_opp[i][j] = p.getzddnum(zdd_child_cap_r);
		captured_piece_type_opp[i][j] = 2;
	      }
	    } else {
	      array_id_opp[i][j] = board_check;
	    }
	    p.undo_action();
	  }
	}
	w->set_opp(nchild, num_of_un, captured_piece_type_opp, array_id_opp);
      } else {
	assert(iter % 2 == 0);
	long long int array_id_opp_e[max_belief_state];

	for(int i = 0; i < num_of_un; i++) {
	  p.make_posi_n(board_belief, i);
	  p.make_posi_opponent();
	  array_id_opp_e[i] = p.getzddnum(zdd_parent_opp);
	}
	w->set_opp(num_of_un, array_id_opp_e);
      }
    }
    
    // cout << "end : worker! : " << w->get_id() << endl;
    // ZDDの経路を辿る
    // 後退解析の表は使わない
    // w->set_path_length(length);     
    lck.lock();//ロック(deq_outputに触るため)
    deq_output.push_front(w); // 1つのスレッドしか触っちゃいけない     
    lck.unlock();//ロック解除
    cv_boss.notify_one();//起きれるやつがいたら起こしてからコールバック, notify_one()はアンロック状態で実行されないといけない
    //cv_bossで現在waitをcallしているスレッド1つに信号が行く。
  }
}

int main(int argc, char *argv[]) {
  //通常の処理
  //argv[2~5] : (i, j, k, l)
  int iteration = atoi(argv[1]);
  int num_b = atoi(argv[2]), num_r = atoi(argv[3]), num_eb = atoi(argv[4]), num_er = atoi(argv[5]);

  string filename_self = "data/db/" + base[0] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_enemy = "data/db/" + base[1] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_self_opp = "data/db/" + base[0] + '_' + to_string(num_eb) + '-' + to_string(num_er) + '-' + to_string(num_b) + '-' + to_string(num_r) + ".bin";
  string filename_enemy_opp = "data/db/" + base[1] + '_' + to_string(num_eb) + '-' + to_string(num_er) + '-' + to_string(num_b) + '-' + to_string(num_r) + ".bin";
  string filename_enemy_cap_b = "data/db/" + base[1] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb - 1) + '-' + to_string(num_er) + ".bin";
  string filename_enemy_cap_r = "data/db/" + base[1] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er - 1) + ".bin";
  string filename_self_cap_b = "data/db/" + base[0] + '_' + to_string(num_b - 1) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_self_cap_r = "data/db/" + base[0] + '_' + to_string(num_b) + '-' + to_string(num_r - 1) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_enemy_opp_cap_b = "data/db/" + base[1] + '_' + to_string(num_eb) + '-' + to_string(num_er) + '-' + to_string(num_b - 1) + '-' + to_string(num_r) + ".bin";
  string filename_enemy_opp_cap_r = "data/db/" + base[1] + '_' + to_string(num_eb) + '-' + to_string(num_er) + '-' + to_string(num_b) + '-' + to_string(num_r - 1) + ".bin";
  string filename_self_opp_cap_b = "data/db/" + base[0] + '_' + to_string(num_eb - 1) + '-' + to_string(num_er) + '-' + to_string(num_b) + '-' + to_string(num_r) + ".bin";
  string filename_self_opp_cap_r = "data/db/" + base[0] + '_' + to_string(num_eb) + '-' + to_string(num_er - 1) + '-' + to_string(num_b) + '-' + to_string(num_r) + ".bin";
  
  unsigned long long int placement = placement_count[num_b][num_r][num_eb + num_er];
  unsigned long long int placement_opp = placement_count[num_eb][num_er][num_b + num_r];
  unsigned long long int placement_self_cap_b = placement_count[num_b - 1][num_r][num_eb + num_er];
  unsigned long long int placement_self_cap_r = placement_count[num_b][num_r - 1][num_eb + num_er];
  unsigned long long int placement_enemy_cap = placement_count[num_b][num_r][num_eb + num_er - 1];
  unsigned long long int placement_self_opp_cap_b = placement_count[num_eb - 1][num_er][num_b + num_r];
  unsigned long long int placement_self_opp_cap_r = placement_count[num_eb][num_er - 1][num_b + num_r];
  unsigned long long int placement_enemy_opp_cap = placement_count[num_eb][num_er][num_b + num_r - 1];
  
  unique_ptr<ZDD> zdd = make_unique<ZDD>(num_b, num_r, num_eb + num_er);
  unique_ptr<ZDD> zdd_opp = make_unique<ZDD>(num_eb, num_er, num_b + num_r);
  unique_ptr<ZDD> zdd_enemy_cap = make_unique<ZDD>(num_b, num_r, num_eb + num_er - 1);
  unique_ptr<ZDD> zdd_self_cap_b = make_unique<ZDD>(num_b - 1, num_r, num_eb + num_er);
  unique_ptr<ZDD> zdd_self_cap_r = make_unique<ZDD>(num_b, num_r - 1, num_eb + num_er);
  unique_ptr<ZDD> zdd_enemy_opp_cap = make_unique<ZDD>(num_eb, num_er, num_b + num_r - 1);
  unique_ptr<ZDD> zdd_self_opp_cap_b = make_unique<ZDD>(num_eb - 1, num_er, num_b + num_r);
  unique_ptr<ZDD> zdd_self_opp_cap_r = make_unique<ZDD>(num_eb, num_er - 1, num_b + num_r);
  
  Table table_self(iteration - 2, filename_self.c_str(), 2, placement);
  Table table_enemy(iteration - 1, filename_enemy.c_str(), 2, placement);
  Table table_self_opp(iteration - 2, filename_self_opp.c_str(), 2, placement_opp);
  Table table_enemy_opp(iteration - 1, filename_enemy_opp.c_str(), 2, placement_opp);
  Table table_self_cap_b(0, filename_self_cap_b.c_str(), 2, placement_self_cap_b);
  Table table_self_cap_r(0, filename_self_cap_r.c_str(), 2, placement_self_cap_r);
  Table table_enemy_cap_b(0, filename_enemy_cap_b.c_str(), 2, placement_enemy_cap);
  Table table_enemy_cap_r(0, filename_enemy_cap_r.c_str(), 2, placement_enemy_cap);
  Table table_self_opp_cap_b(0, filename_self_opp_cap_b.c_str(), 2, placement_self_opp_cap_b);
  Table table_self_opp_cap_r(0, filename_self_opp_cap_r.c_str(), 2, placement_self_opp_cap_r);
  Table table_enemy_opp_cap_b(0, filename_enemy_opp_cap_b.c_str(), 2, placement_enemy_opp_cap);
  Table table_enemy_opp_cap_r(0, filename_enemy_opp_cap_r.c_str(), 2, placement_enemy_opp_cap);
    
  while(true) {
    iteration++;

    if(num_b == num_eb && num_r == num_er) {
      std::cout << "iter > " << iteration << ": (" << num_b << ", " << num_r << ", " << num_eb << ", " << num_er << ")" << endl;
      
      // flag indicating whether the worker has finished its work
      // always set to false before creating the boss
      flag_worker_quit = false;
      
      if(iteration % 2 == 1) {
	thread th_boss(boss, iteration, num_b, num_r, num_eb, num_er,
		       ref(table_self), filename_self.c_str(),
		       cref(table_enemy), cref(table_enemy_cap_b), cref(table_enemy_cap_r),
		       cref(table_enemy));  //boss側作る
	
	thread th_worker[nworker];  //worker側を作る
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid] = thread(worker, iteration, num_b, num_r, num_eb, num_er,
				       cref(*zdd), cref(*zdd_enemy_cap), cref(*zdd_enemy_cap),
				       cref(*zdd));   //ここでworker()を呼び出す
	}
	
	//終了処理
	th_boss.join(); 
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid].join();
	}
      } else {
	thread th_boss(boss, iteration, num_b, num_r, num_eb, num_er,
		       ref(table_enemy), filename_enemy.c_str(),
		       cref(table_self), cref(table_self_cap_b), cref(table_self_cap_r),
		       cref(table_self));
	
	thread th_worker[nworker];
	for(int workerid = 0; workerid < nworker; workerid++) {
	  th_worker[workerid] = thread(worker, iteration, num_b, num_r, num_eb, num_er,
				       cref(*zdd), cref(*zdd_self_cap_b), cref(*zdd_self_cap_r),
				       cref(*zdd));
	}
	
	//終了処理
	th_boss.join(); 
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid].join();
	}
      }
      
      // cout << "nwin  = " << nwin << endl;
      // cout << "nlose = " << nlose << endl;
      // cout << "ncan_win_lose  =  " << ncan_win_lose << endl;
      // cout << "ncan_win = " << ncan_win << endl;
      // cout << "ncan_lose = " << ncan_lose << endl;
      // cout << endl;
      
      if(count_changes == 0) {
	cout << "analysis finished" << endl;
	break;
      }
    } else {
      assert(num_b != num_eb || num_r != num_er);
      // First, process (num_b, num_r, num_eb, num_er)
      std::cout << "iter > " << iteration << ": (" << num_b << ", " << num_r << ", " << num_eb << ", " << num_er << ")" << endl;
      
      // flag indicating whether the worker has finished its work
      // always set to false before creating the boss
      flag_worker_quit = false;
      
      if(iteration % 2 == 1) {
	thread th_boss(boss, iteration, num_b, num_r, num_eb, num_er,
		       ref(table_self), filename_self.c_str(),
		       cref(table_enemy), cref(table_enemy_cap_b), cref(table_enemy_cap_r),
		       cref(table_enemy_opp));  //boss側作る
	
	thread th_worker[nworker];  //worker側を作る
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid] = thread(worker, iteration, num_b, num_r, num_eb, num_er,
				       cref(*zdd), cref(*zdd_enemy_cap), cref(*zdd_enemy_cap),
				       cref(*zdd_opp));   //ここでworker()を呼び出す
	}
	
	//終了処理
	th_boss.join(); 
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid].join();
	}
      } else {
	thread th_boss(boss, iteration, num_b, num_r, num_eb, num_er,
		       ref(table_enemy), filename_enemy.c_str(),
		       cref(table_self), cref(table_self_cap_b), cref(table_self_cap_r),
		       cref(table_self_opp));
	
	thread th_worker[nworker];
	for(int workerid = 0; workerid < nworker; workerid++) {
	  th_worker[workerid] = thread(worker, iteration, num_b, num_r, num_eb, num_er,
				       cref(*zdd), cref(*zdd_self_cap_b), cref(*zdd_self_cap_r),
				       cref(*zdd_opp));
	}
	
	//終了処理
	th_boss.join(); 
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid].join();
	}
      }

      // Next, process (num_eb, num_er, num_b, num_r)
      std::cout << "iter > " << iteration << ": (" << num_eb << ", " << num_er << ", " << num_b << ", " << num_r << ")" << endl;
      int count_changes_copy = count_changes;
      
      // flag indicating whether the worker has finished its work
      // always set to false before creating the boss
      flag_worker_quit = false;
      
      if(iteration % 2 == 1) {
	thread th_boss(boss, iteration, num_eb, num_er, num_b, num_r,
		       ref(table_self_opp), filename_self_opp.c_str(),
		       cref(table_enemy_opp), cref(table_enemy_opp_cap_b), cref(table_enemy_opp_cap_r),
		       cref(table_enemy));  //boss側作る
	
	thread th_worker[nworker];  //worker側を作る
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid] = thread(worker, iteration, num_eb, num_er, num_b, num_r,
				       cref(*zdd_opp), cref(*zdd_enemy_opp_cap), cref(*zdd_enemy_opp_cap),
				       cref(*zdd));   //ここでworker()を呼び出す
	}
	
	//終了処理
	th_boss.join(); 
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid].join();
	}
      } else {
	thread th_boss(boss, iteration, num_eb, num_er, num_b, num_r,
		       ref(table_enemy_opp), filename_enemy_opp.c_str(),
		       cref(table_self_opp), cref(table_self_opp_cap_b), cref(table_self_opp_cap_r),
		       cref(table_self));
	
	thread th_worker[nworker];
	for(int workerid = 0; workerid < nworker; workerid++) {
	  th_worker[workerid] = thread(worker, iteration, num_eb, num_er, num_b, num_r,
				       cref(*zdd_opp), cref(*zdd_self_opp_cap_b), cref(*zdd_self_opp_cap_r),
				       cref(*zdd));
	}
	
	//終了処理
	th_boss.join(); 
	for(int workerid = 0; workerid < nworker; workerid++){
	  th_worker[workerid].join();
	}
      }
      
      // cout << "nwin  = " << nwin << endl;
      // cout << "nlose = " << nlose << endl;
      // cout << "ncan_win_lose  =  " << ncan_win_lose << endl;
      // cout << "ncan_win = " << ncan_win << endl;
      // cout << "ncan_lose = " << ncan_lose << endl;
      // cout << endl;
      
      if(count_changes == 0 && count_changes_copy == 0) {
	cout << "analysis finished" << endl;
	break;
      }
    }
  }

  return 0;
}
  
// g++ -O2 -o zdd.exe zdd_bg.cpp -std=c++11
// ./zdd.exe > res.txt &

// g++ -o gened gened.cpp posi.cpp zdd.cpp
// ./gened 0 table.bin db > res_iter0.txt 2>&1 &
// bash batch.sh > res_iter-.txt 2>&1 &

// g++ -o gened gened.cpp posi.cpp zdd.cpp
// bash batch.sh > database?-?-?-?.txt 2>&1 &

// g++ -DUSE_PURPLE -o gened gened.cpp posi.cpp zdd.cpp

// g++ -o gened gened.cpp database.cpp posi.cpp zdd.cpp
// bash batch.sh | tee database?-?-?-?.txt
