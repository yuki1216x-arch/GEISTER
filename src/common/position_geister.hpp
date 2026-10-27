#ifndef POSITION_GEISTER_H
#define POSITION_GEISTER_H
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

// Piece encoding, display character, and count contributions for one square.
struct SquareInfo {
  uint8_t piece;
  char display_character;
  unsigned char blue_count;
  unsigned char red_count;
  unsigned char unknown_count;

  bool has_same_square_info(const SquareInfo &other) const noexcept {
    return(piece == other.piece && display_character == other.display_character && blue_count == other.blue_count && red_count == other.red_count && unknown_count == other.unknown_count);
  }
};

// Object IDs: empty, unknown opponent, self blue/red, opponent blue/red.
constexpr SquareInfo square_info_by_object_id[6] = {
  {(empty|self)   , '.', 0, 0, 0},   //0
  {(unknown|enemy), 'u', 0, 0, 1},   //1
  {(blue|self)    , 'B', 1, 0, 0},   //2
  {(red|self)     , 'R', 0, 1, 0},   //3
  {(blue|enemy)   , 'b', 1, 0, 0},   //4
  {(red|enemy)    , 'r', 0, 1, 0}    //5
};					  

// Move endpoints; destination 100 denotes an escape from the board.
class Action{
private:
  int m_source_square;
  int m_destination_square;
public:
  Action() noexcept : m_source_square(-1), m_destination_square(-1) {}
  Action(int source_square, int destination_square) noexcept : m_source_square(source_square), m_destination_square(destination_square) {}
  int get_source_square() const noexcept { return m_source_square; }
  int get_destination_square() const noexcept { return m_destination_square; }
};

// Board state in the current viewpoint, with snapshots for undoing actions.
class Position {
private:
  struct Snapshot {
    SquareInfo m_board[36];   // current square contents
    
    int m_self_blue_count;
    int m_self_red_count;
    int m_opponent_blue_count;
    int m_opponent_red_count;
  };

  SquareInfo m_board[36];   // current square contents
  
  int m_self_blue_count;
  int m_self_red_count;
  int m_opponent_blue_count;
  int m_opponent_red_count;
  
  stack<Snapshot> m_history;   // snapshots saved by apply_action
  
public:
  Position() noexcept;
  Position(unsigned long long int configuration_id, const ZDD& zdd,
       int self_blue_count, int self_red_count, int opponent_blue_count, int opponent_red_count) noexcept;
  // Decode a configuration ID and set the piece counts.
  void load_configuration(unsigned long long int configuration_id, const ZDD& zdd,
		 int self_blue_count, int self_red_count, int opponent_blue_count, int opponent_red_count) noexcept;
  // Load one fully specified board; retain the existing piece counts.
  void load_compatible_configuration(unsigned char compatible_boards[70][36], int n) noexcept;
  // Swap owners and counts, rotate 180 degrees, then hide opponent colors.
  void switch_to_opponent_view() noexcept;
  // Hide opponent colors while keeping ownership, counts, and coordinates.
  void hide_opponent_colors() noexcept;
  void print() const noexcept;
  // Odd turn selectors move self; even selectors move the opponent.
  int generate_legal_actions(Action actions[1000], int turn_selector) noexcept;
  // Return the object ID associated with a square encoding.
  int get_object_id(const SquareInfo& square) const noexcept;
  // Encode the current board using the supplied piece-count ZDD.
  unsigned long long int get_zdd_id(const ZDD& zdd) const noexcept;
  // Enumerate compatible opponent color assignments in the existing order.
  int enumerate_compatible_configurations(unsigned char compatible_boards[70][36]) const noexcept;
  // Save a snapshot, then apply an action. Return 0/1/2 for no/blue/red capture; -1/-2 for self win/loss.
  int apply_action(const Action& action) noexcept;
  // Restore the most recent snapshot, including all piece counts.
  void undo_action() noexcept;
};

#endif
