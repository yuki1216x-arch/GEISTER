#include <memory>
#include <iostream>
#include <vector>
#include <exception>
#include <cassert>
#include "../common/node.hpp"
#include "../common/table.hpp"
#include "../common/zdd_geister.hpp"
#include "../common/posi_geister.hpp"

using std::unique_ptr;
using std::make_unique;
using std::vector;

constexpr int MAX_DEPTH = 144; // 144

const string base[2] = {"self_table", "enemy_table"};

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

void search(Node& cur, vector<unsigned char>& array_objid, const ZDD& zdd, int depth, unsigned long long int& leaf_id, int num_b, int num_r, int num_eb, int num_er) {
  if(depth == MAX_DEPTH) {
    assert(array_objid.size() == 36);
    unsigned char array_objid_zdd[36];

    zdd.compute_array(leaf_id, array_objid_zdd, 36);
    
    for(size_t i = 0; i < array_objid.size(); i++) {
      if(array_objid[i] != array_objid_zdd[i]) {
	std::cerr << "zdd error, id = " << leaf_id << ", locid = " << i << endl;
	std::cerr << "dfp = " << +array_objid[i] << ", zdd = " << +array_objid_zdd[i] << endl;
	std::terminate();
      }
    }

    unsigned long long int array_id = zdd.compute_id(array_objid.data(), array_objid.size());
    if(leaf_id != array_id ) {
      std::cerr << "id error, id = " << leaf_id << endl;
      std::terminate();
    }

    if(leaf_id % 100000000 == 0) {
      cout << "ok! id = " << leaf_id << ": ";
      for(char x : array_objid) {
	cout << +x;
      }
      cout << endl;
    }
    
    leaf_id++;
    return;
  }

  for(int x = 0; x < 2; x++) { // for each branch (0 and 1)
    if(cur.IsNextLeaf0(depth, x, num_b, num_r, num_eb + num_er)) { // if the x-branch leads to the 0-terminal node
      /*cout << depth << ", " << x << "false: ";
      for(size_t i = 0; i < array_objid.size(); i++) {
	cout << array_objid[i];
      }
      cout << endl;*/
      continue;
    } else { // if current node has children
      unique_ptr<Node> c = make_unique<Node>(cur, x);

      if(x == 0) {
	search(*c, array_objid, zdd, depth + 1, leaf_id, num_b, num_r, num_eb, num_er); // left
      } else {
	array_objid.push_back(cur.get_loc_stateid());
	search(*c, array_objid, zdd, depth + 1, leaf_id, num_b, num_r, num_eb, num_er); // right
	array_objid.pop_back();
      }
    }
  }
  return;
}

bool detect_invalid1(int parent_val, int child_val[32][70], int nchild, int num_of_un,
				      int play_to_move, int num_b, int num_r, int num_eb, int num_er) {
  assert(parent_val == v_win || parent_val == v_can_lose);

  assert(num_of_un > 0 && num_of_un <= 70);
  assert(nchild > 0 && nchild < 32);
  
  if(parent_val == v_win) { // win
    if(play_to_move == 1) {
      for(int j = 0; j < nchild; j++) {
	int num_win = 0;
	for(int m = 0; m < num_of_un; m++) {
	  if(child_val[j][m] == v_win) num_win++;
	}
	if(num_win == num_of_un) return false;
      }
      return true;
    } else {
      for(int j = 0; j < nchild; j++) {
	for(int m = 0; m < num_of_un; m++) {
	  if(child_val[j][m] != v_win) return true;
	}
      }
      return false;
    }
  } else { // can lose
    assert(parent_val == v_can_lose);
    if(play_to_move == 1) {
      for(int j = 0; j < nchild; j++) {
	int num_canlose = 0;
	for(int m = 0; m < num_of_un; m++) {
	  if(child_val[j][m] == v_can_lose || child_val[j][m] == v_lose) num_canlose++;
	}
	if(num_canlose < 1) return true;
      }
      return false;
    } else {
      for(int j = 0; j < nchild; j++) {
	for(int m = 0; m < num_of_un; m++) {
	  if(child_val[j][m] == v_can_lose || child_val[j][m] == v_lose) {
	    return false;
	  }
	}
      }
      return true;
    }
  }
}

bool detect_invalid2(int parent_val, int child_val[32][70], int child_opp_val[70], int nchild, int num_of_un,
					int play_to_move, int num_b, int num_r, int num_eb, int num_er) {
  assert(parent_val == v_lose);

  assert(num_of_un > 0 && num_of_un <= 70);
  assert(nchild > 0 && nchild < 32);
  
  if(play_to_move == 1) { // lose
    for(int j = 0; j < nchild; j++) {
      for(int m = 0; m < num_of_un; m++) {
	if(child_val[j][m] != v_lose) return true;
      }
    }
    return false;
  } else {
    assert(play_to_move == 2);
    int num_lose = 0;
    for(int m = 0; m < num_of_un; m++) {
      if(child_opp_val[m] == v_win) num_lose++;
    }
    if(num_lose == num_of_un) return false;
    else return true;
  }
}

int main(int argc, char *argv[]) {
  int num_b = atoi(argv[1]), num_r = atoi(argv[2]), num_eb = atoi(argv[3]), num_er = atoi(argv[4]); // player perspective (0: white, 1: black)
  unsigned long long int id = 0;
  string filename_self = "data/db/" + base[0] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_enemy = "data/db/" + base[1] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_self_opp = "data/db/" + base[0] + '_' + to_string(num_eb) + '-' + to_string(num_er) + '-' + to_string(num_b) + '-' + to_string(num_r) + ".bin";
  string filename_enemy_opp = "data/db/" + base[1] + '_' + to_string(num_eb) + '-' + to_string(num_er) + '-' + to_string(num_b) + '-' + to_string(num_r) + ".bin";
  string filename_enemy_cap_b = "data/db/" + base[1] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb - 1) + '-' + to_string(num_er) + ".bin";
  string filename_enemy_cap_r = "data/db/" + base[1] + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er - 1) + ".bin";
  string filename_self_cap_b = "data/db/" + base[0] + '_' + to_string(num_b - 1) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  string filename_self_cap_r = "data/db/" + base[0] + '_' + to_string(num_b) + '-' + to_string(num_r - 1) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  
  unsigned long long int placement = placement_count[num_b][num_r][num_eb + num_er];
  unsigned long long int placement_opp = placement_count[num_eb][num_er][num_b + num_r];
  unsigned long long int placement_self_cap_b = placement_count[num_b - 1][num_r][num_eb + num_er];
  unsigned long long int placement_self_cap_r = placement_count[num_b][num_r - 1][num_eb + num_er];
  unsigned long long int placement_enemy_cap = placement_count[num_b][num_r][num_eb + num_er - 1];

  unique_ptr<ZDD> zdd = make_unique<ZDD>(num_b, num_r, num_eb + num_er);
  unique_ptr<ZDD> zdd_opp = make_unique<ZDD>(num_eb, num_er, num_b + num_r);
  unique_ptr<ZDD> zdd_enemy_cap = make_unique<ZDD>(num_b, num_r, num_eb + num_er - 1);
  unique_ptr<ZDD> zdd_self_cap_b = make_unique<ZDD>(num_b - 1, num_r, num_eb + num_er);
  unique_ptr<ZDD> zdd_self_cap_r = make_unique<ZDD>(num_b, num_r - 1, num_eb + num_er);
  
  {
    unique_ptr<Node> root = make_unique<Node>();
    vector<unsigned char> array_objid;
  
    search(*root, array_objid, *zdd, 0, id, num_b, num_r, num_eb, num_er);
  }

  cout << "leaf num: " << id << endl;

  cout << "zdd is ok. (piece : " << num_b << "," << num_r << "," << num_eb << "," << num_er << ")" << endl;

  // return 0;

  // from here, verify parent-child relationships
  Table table_self(0, filename_self.c_str(), 4, placement);
  Table table_enemy(0, filename_enemy.c_str(), 4, placement);
  Table table_self_opp(0, filename_self_opp.c_str(), 4, placement_opp);
  Table table_enemy_opp(0, filename_enemy_opp.c_str(), 4, placement_opp);
  Table table_self_cap_b(0, filename_self_cap_b.c_str(), 4, placement_self_cap_b);
  Table table_self_cap_r(0, filename_self_cap_r.c_str(), 4, placement_self_cap_r);
  Table table_enemy_cap_b(0, filename_enemy_cap_b.c_str(), 4, placement_enemy_cap);
  Table table_enemy_cap_r(0, filename_enemy_cap_r.c_str(), 4, placement_enemy_cap);
  
  Posi p;
  bool is_error = false;

  for(int play_to_move = 1; play_to_move <= 2; play_to_move++) {
    for(unsigned long long int i = 0ULL; i < placement; i++) {
      p.make_posi(i, *zdd, num_b, num_r, num_eb, num_er);
      Action actions[32] = {};
      unsigned char board_belief[70][36] = {};
      int nchild = p.compute_actions(actions, play_to_move);
      int num_of_un = p.getunknowninfo(board_belief);
    
      int parent_val;
      if(play_to_move == 1) parent_val = table_self.get(i);
      else parent_val = table_enemy.get(i);
      // if(parent_val < 0 || parent_val > 5) std::cout << "parent = " << parent_val << endl;
      assert(parent_val >= 0 && parent_val <= 5);

      assert(nchild > 0);
      int child_val[32][70] = {};
      int child_opp_val[70] = {};
      for(int j = 0; j < nchild; j++) {
	for(int m = 0; m < num_of_un; m++) {
	  p.make_posi_n(board_belief, m);
	  int board_check = p.make_action(actions[j]);
	  if(board_check >= 0) { // Non-terminal node
	    p.make_posi_myself();
	    if(play_to_move == 1) {
	      if(board_check == 0) child_val[j][m] = table_enemy.get(p.getzddnum(*zdd)); // retrieve the value from the child's table
	      else if(board_check == 1) child_val[j][m] = table_enemy_cap_b.get(p.getzddnum(*zdd_enemy_cap)); // win (player's perspective)
	      else {
		assert(board_check == 2);
		child_val[j][m] = table_enemy_cap_r.get(p.getzddnum(*zdd_enemy_cap)); // lose (player's perspective)
	      }
	    } else {
	      if(board_check == 0) child_val[j][m] = table_self.get(p.getzddnum(*zdd));
	      else if(board_check == 1) child_val[j][m] = table_self_cap_b.get(p.getzddnum(*zdd_self_cap_b));
	      else {
		assert(board_check == 2);
		child_val[j][m] = table_self_cap_r.get(p.getzddnum(*zdd_self_cap_r));
	      }
	    }
	  } else {
	    if(board_check == -1) child_val[j][m] = v_win;
	    else {
	      assert(board_check == -2);
	      child_val[j][m] = v_lose;
	    }
	  }
	  p.undo_action(); //undo
	}
      }
      for(int m = 0; m < num_of_un; m++) {
	p.make_posi_n(board_belief, m);
	p.make_posi_opponent();
	if(play_to_move == 1) child_opp_val[m] = table_enemy_opp.get(p.getzddnum(*zdd_opp));
	else child_opp_val[m] = table_self_opp.get(p.getzddnum(*zdd_opp));
      }
      if(parent_val == v_win || parent_val == v_can_lose) {
	if(detect_invalid1(parent_val, child_val, nchild, num_of_un, play_to_move, num_b, num_r, num_eb, num_er)) {
	  is_error = true;
	  std::cout << "label error1, id = " << i << ", parent = " << parent_val << endl;
	  std::cout << "child = ";
	  for(int j = 0; j < nchild; j++) {
	    for(int m = 0; m < num_of_un; m++) {
	      std::cout << child_val[j][m];
	    }
	    if(j != nchild - 1) std::cerr << " ";
	  }
	  std::cout << endl;
	  p.make_posi(i, *zdd, num_b, num_r, num_eb, num_er);
	  p.print();
	  break;
	}
      }
      if(parent_val == v_lose) {
	if(detect_invalid2(parent_val, child_val, child_opp_val, nchild, num_of_un, play_to_move, num_b, num_r, num_eb, num_er)) {
	  is_error = true;
	  std::cout << "label error2, id = " << i << ", parent = " << parent_val << endl;
	  std::cout << "child = ";
	  if(play_to_move == 1) {
	    for(int j = 0; j < nchild; j++) {
	      for(int m = 0; m < num_of_un; m++) {
		std::cout << child_val[j][m];
	      }
	      if(j != nchild - 1) std::cerr << " ";
	    }
	  } else {
	    for(int m = 0; m < nchild; m++) {
	      cout << child_opp_val[m];
	    }
	  }
	  std::cout << endl;
	  p.make_posi(i, *zdd, num_b, num_r, num_eb, num_er);
	  p.print();
	  break;
	}
      }
      if(i % 100000000 == 0) std::cout << "label ok! id = " << i << endl;
    }
  }
  if(is_error) std::terminate();
  return 0;
}
