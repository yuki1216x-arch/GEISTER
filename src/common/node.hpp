#ifndef INCLUDE_NODE_H
#define INCLUDE_NODE_H

#include <exception>
#include <memory>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cassert>

using std::cout;
using std::endl;
using std::max;
using std::min;
using std::unique_ptr;

constexpr uint8_t PLAYER_MASK = 0b0001;   // bit0
constexpr uint8_t COLOR_MASK  = 0b0110;   // bit1-2

constexpr uint8_t self    = 0b0000;
constexpr uint8_t enemy   = 0b0001;
constexpr uint8_t empty   = 0b0000;
constexpr uint8_t blue    = 0b0010;
constexpr uint8_t red     = 0b0100;
constexpr uint8_t unknown = 0b0110;

class LocInfo_vali {
  uint8_t m_piece;
  int m_nb; // number of blue piece
  int m_nr; // number of red piece
  int m_uk; // number of unknown piece

public:
  LocInfo_vali(char piece, char nb, char nr, char uk) noexcept: m_piece(piece), m_nb(nb), m_nr(nr), m_uk(uk) { assert(ok()); }
  int get_piece() const noexcept { return m_piece; }
  int get_nb() const noexcept { return m_nb; }
  int get_nr() const noexcept { return m_nr; }
  int get_uk() const noexcept { return m_uk; }
  bool ok() const noexcept;
  bool operator==(const LocInfo_vali &o) const noexcept {
    assert(o.ok());
    return (m_piece == o.m_piece && m_nb == o.m_nb && m_nr == o.m_nr && m_uk == o.m_uk);
  }
};

class Node {
  int m_nb; // Number of blue pieces up to the current node
  int m_nr; // Number of red pieces up to the current node
  int m_ne; // Number of unknown pieces up to the current node
  int m_f; // whether a piece has been placed on this square
  int m_locid;
  int m_loc_stateid;
  unsigned long long int m_num; // number of reachable leaf nodes with value 1 (number of possible board configurations)
  bool m_visited; // for dfs to compute path length
  int m_lengthmax; // holds the maximum path length value
  int m_lengthmin; // holds the minimum path length value
  unsigned long long int m_lengthsum; // total path length for computing the average path length

public:
  Node()
    : m_nb(0), m_nr(0), m_ne(0), m_f(0),
      m_locid(0), m_loc_stateid(0), m_num(0), m_visited(false),
      m_lengthmax(0), m_lengthmin(0), m_lengthsum(0) {}
  Node(char placeholder, unsigned long long int num, bool visited, int lengthmax, int lengthmin, unsigned long long int lengthsum)
    : m_nb(placeholder), m_nr(placeholder), m_ne(placeholder), m_f(placeholder),
      m_locid(0), m_loc_stateid(0), m_num(num), m_visited(visited),
      m_lengthmax(lengthmax), m_lengthmin(lengthmin), m_lengthsum(lengthsum) {}
  Node(Node& cur, int x) noexcept;
  Node* m_left;
  Node* m_right;
  
  bool IsNextLeaf0(int depth, int x, int board_nb, int board_nr, int board_ne) const noexcept;
  int get_locid() const noexcept { return m_locid; }
  int get_loc_stateid() const noexcept { return m_loc_stateid; }
  unsigned long long int get_num() const noexcept { return m_num; }
  int get_f() const noexcept { return m_f; }
  int get_lengthmax() const noexcept { return m_lengthmax; }
  int get_lengthmin() const noexcept { return m_lengthmin; }
  unsigned long long int get_lengthsum() const noexcept { return m_lengthsum; }
  void dfs() noexcept {
    assert(m_left != nullptr && m_right != nullptr);
    if(!m_left->m_visited) m_left->dfs();
    if(!m_right->m_visited) m_right->dfs();
    m_num = m_left->m_num + m_right->m_num;
    m_lengthmax = max(m_left->m_lengthmax, m_right->m_lengthmax) + 1;         
    m_lengthmin = min(m_left->m_lengthmin, m_right->m_lengthmin) + 1;
    m_lengthsum = m_left->m_lengthsum + m_right->m_lengthsum + m_num;
    m_visited = true;
  }
  bool ExistEquivalentNode(Node& c) const noexcept {
    return m_f == c.m_f && m_nb == c.m_nb && m_nr == c.m_nr && m_ne == c.m_ne;
  }
  void setRedundantNode() noexcept { m_f = 40; }
  bool IsRedundantNode() const noexcept { return m_f == 40; }
  bool IsLeaf0() const noexcept { return m_f == 50; }
  bool IsLeaf1() const noexcept { return m_f == 60; }
};

#endif
