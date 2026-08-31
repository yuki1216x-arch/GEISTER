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

//---.exe iter read_file write_file
int main(int argc, char *argv[]) {
  int player_to_move = atoi(argv[1]); // player to move
  int num_b = atoi(argv[2]), num_r = atoi(argv[3]), num_eb = atoi(argv[4]), num_er = atoi(argv[5]); //
  string base_filename = base[player_to_move - 1];
  string read_filename_str = "data/db/" + base_filename + '_' + to_string(num_b) + '-' + to_string(num_r) + '-' + to_string(num_eb) + '-' + to_string(num_er) + ".bin";
  
  unique_ptr<ZDD> zdd_check = make_unique<ZDD>(num_b, num_r, num_eb + num_er);
  Posi p;
  
  unsigned long long int tbl_dbg[100];
  mt19937_64 rng(high_resolution_clock::now().time_since_epoch().count());
  uniform_int_distribution<unsigned long long int> dist(0, placement_count[num_b][num_r][num_eb + num_er] - 1);
  for(int k = 0; k < 100; k++) {
    tbl_dbg[k] = dist(rng);
  }

  std::ifstream file(read_filename_str, std::ios::binary);
  if (!file) {
    std::cerr << "Failed to open the file\n";
    return 1;
  }

  for(int k = 0; k < 100; k++) {
    unsigned long long int seek_id = (tbl_dbg[k] / 2) + 1;
    file.seekg(static_cast<std::streamoff>(seek_id)); // ← byte offset to read (0-based)

    char byte;
    file.read(&byte, 1);
    
    if(tbl_dbg[k] % 2 == 1) byte = byte >> 4U;
    unsigned int val = byte & 15U;

    if(val != v_lose) continue;
    
    std::cout << "id of this configuration：" << tbl_dbg[k] << endl;

    p.make_posi(tbl_dbg[k], *zdd_check, num_b, num_r, num_eb, num_er);
    p.print();
    
    std::cout << "label index = " << val;
    
    if (val == v_unknown) std::cout << ", label: unknown" << endl;
    else if (val == v_win) std::cout << ", label：win" << endl;
    else if (val == v_lose) std::cout << ", label：lose" << endl;
    else {
      assert(val == v_can_lose || val == 5);
      std::cout << ", label：can lose" << endl;
    }
  }

  file.close();

  return 0;
}
