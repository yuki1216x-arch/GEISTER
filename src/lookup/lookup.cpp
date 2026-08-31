#include <memory>
#include <chrono>
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "table.hpp"
#include "zdd_geister.hpp"
#include "posi_geister.hpp"

using std::unique_ptr;
using std::make_unique;
using std::mt19937_64;
using std::uniform_int_distribution;
using std::chrono::high_resolution_clock;
using std::to_string;

std::vector<string> split(const string text, const char delimiter='/') {
  std::vector<string> columns;

  if (text.empty()) {
    return columns;
  }

  std::stringstream stream{text};
  string buff;
  while (getline(stream, buff, delimiter)) {
    columns.push_back(buff);
  }
  return columns;
}

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

unsigned long long int getzddnum(const ZDD& zdd, string fen_str) noexcept {
  unsigned char array_objid[36] = {};
  std::vector<string> cols = split(fen_str);
  int square_id = 35;
  assert(cols.size() == 6);

  for (std::size_t i = 0; i < cols.size(); i++) {
    for (std::size_t j = 0; j < cols[i].size(); j++) {
      assert(i >= 0 && i <= 5);
      if (cols[i][j] != tbl_objid2locinfo[1].piece_ch && cols[i][j] != tbl_objid2locinfo[2].piece_ch && cols[i][j] != tbl_objid2locinfo[3].piece_ch) {
	assert(cols[i][j] - '0' >= 1 && cols[i][j] - '0' <= square_id % 6 + 1);
	for (int k = cols[i][j] - '0'; k > 0; k--) {
	  array_objid[square_id] = 0;
	  square_id--;
	}
      } else {
	if (cols[i][j] == tbl_objid2locinfo[1].piece_ch) array_objid[square_id] = 1;
	else if (cols[i][j] == tbl_objid2locinfo[2].piece_ch) array_objid[square_id] = 2;
	else {
	  assert(cols[i][j] == tbl_objid2locinfo[3].piece_ch);
	  array_objid[square_id] = 3;
	}
	square_id--;
      }
    }
  }
  assert(square_id == -1);
  return zdd.compute_id(array_objid, 36);
}

//---.exe iter read_file write_file
int main(int argc, char *argv[]) {
  int player_to_move = atoi(argv[1]); // player to move
  int num_b = atoi(argv[2]), num_r = atoi(argv[3]), num_eb = atoi(argv[4]), num_er = atoi(argv[5]); //
  string base_filename = base[player_to_move - 1];
  string read_filename_str = "data/db/" + base_filename + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  
  unique_ptr<ZDD> zdd_check = make_unique<ZDD>(num_b, num_r, num_eb + num_er);
  Posi p;

  unsigned long long int position_id = getzddnum(*zdd_check, argv[6]);
  unsigned long long int seek_id = (position_id / 2) + 8;

  std::cout << "id of this configuration：" << position_id << endl;

  p.make_posi(position_id, *zdd_check, num_b, num_r, num_eb, num_er);
  p.print();

  std::ifstream file(read_filename_str, std::ios::binary);
  if (!file) {
    std::cerr << "Failed to open the file\n";
    return 1;
  }
  
  file.seekg(static_cast<std::streamoff>(seek_id)); // ← byte offset to read (0-based)

  char byte;
  file.read(&byte, 1);

  if(position_id % 2 == 1) byte = byte >> 4U;
  unsigned int val = byte & 15U;

  file.close();

  std::cout << "label index = " << val;

  if (val == v_unknown) std::cout << ", label: unknown" << endl;
  else if (val == v_win) std::cout << ", label：win" << endl;
  else if (val == v_lose) std::cout << ", label：lose" << endl;
  else {
    assert(val == v_can_lose || val == 5);
    std::cout << ", label：can lose" << endl;
  }

  return 0;
}
