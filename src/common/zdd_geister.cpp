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

//#define MAX_NB 4         // 青駒の最大数
//#define MAX_NR 4         // 赤駒の最大数
//#define MAX_NE 8         // 敵駒の最大数
//#define MAX_POINT 36     // ポイント(マス)の数

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
  unique_ptr<Node> root = make_unique<Node>();                 //根節点
  int d = -1;
  Node* n;

  m_N[0].push_back(move(root));   //N[0]に根節点を入れる
  for(int i = 0; i < 36; i++) {
    for(int j = 0; j < 4; j++) {
      d++;
      for(size_t k = 0; k < m_N[d].size(); k++) {
	n = m_N[d][k].get();

	//0,1-枝へ進む
	for(int x = 0; x < 2; x++) {
	  //0-葉へ到達するか
	  if(n->IsNextLeaf0(d, x, board_nb, board_nr, board_ne)) {  //x-枝の先が0-枝のとき
	    if(x == 0) n->m_left = l0.get();
	    else n->m_right = l0.get();
	    // continue;
	  } else if(d == 143) {   //深さ最大のとき
	    if(x == 0) n->m_left = l1.get();
	    else n->m_right = l1.get();
	    // continue;
	  } else {   //枝の先が0-枝でも、深さ最大でもないとき
	    unique_ptr<Node> c = make_unique<Node>(*n, x);   //節点を生成
	    
	    //深さd+1に節点がないなら新しく登録
	    if(m_N[d+1].size() == 0) {     
	      if(x == 0) n->m_left = c.get(); //親と子をつなぐ
	      else n->m_right = c.get();
	      m_N[d+1].push_back(move(c));    //次の深さの節点集合に追加
	      // continue;
	    } else {
	      //等価節点の共有
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

	      /*
	      bool flag = false;  //共有するかどうかのflag(trueなら共有する)
	      unsigned int key = 4096*c-> 2048*c->get_f() + 64*c->get_ne() + 8*c->get_nb() + c->get_nr();    //keyを求める
                    
	      auto it = for_share_node.find(key);      //同じkeyを探す
	      if(it != for_share_node.end()) flag = true; //itが末尾でないなら、同じkeyがあった->共有する
	      
	      //flagがfalseなら共有しない
	      if(!flag) {
		if(m_N[d+1].size() > UINT_MAX) {
		  cerr << "too large size!" << endl;
		  terminate();
		}
		for_share_node.insert(make_pair(key, m_N[d+1].size()));    //新しい要素を挿入
		if(x == 0) n->m_left = c.get();
		else n->m_right = c.get();
		m_N[d+1].push_back(move(c));
	      } else { //flagがtrueの場合、共有する
		if(x == 0) n->m_left = m_N[d+1][it->second];
		else n->m_right = m_N[d+1][it->second];
	      }
	      */
            }   
	  }
	}
      }
    }
  }
  
  int end = 1;
  //冗長節点の削除
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
    // cout << "N[" << i << "]: " << m_N[i].size() << endl;
  }
  cout << "sumad =" << sum << endl;   //節点数
}
