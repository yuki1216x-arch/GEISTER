// Sample 100 Geister configuration IDs and print each position and its label.
#include <memory>
#include <chrono>
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "table.hpp"
#include "zdd_geister.hpp"
#include "position_geister.hpp"

using std::unique_ptr;
using std::make_unique;
using std::mt19937_64;
using std::uniform_int_distribution;
using std::chrono::high_resolution_clock;
using std::to_string;

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

const string table_filename_prefixes[2] = {"self_table", "enemy_table"};

int main(int argc, char *argv[]) {
  if(argc == 2 && string(argv[1]) == "--help") {
    cout << "Usage: " << argv[0]
	 << " PLAYER_TO_MOVE SELF_BLUE SELF_RED OPPONENT_BLUE OPPONENT_RED" << endl;
    cout << "PLAYER_TO_MOVE: 1 = self, 2 = opponent. Sample 100 IDs and print each position and its label." << endl;
    return 0;
  }
  if(argc != 6) {
    cerr << "[error] Expected five arguments. Use --help for usage." << endl;
    return 1;
  }
  if(string(argv[1]) != "1" && string(argv[1]) != "2") {
    cerr << "[error] PLAYER_TO_MOVE must be 1 (self) or 2 (opponent)." << endl;
    return 1;
  }
  
  int player_to_move = atoi(argv[1]);
  int self_blue_count = atoi(argv[2]), self_red_count = atoi(argv[3]), opponent_blue_count = atoi(argv[4]), opponent_red_count = atoi(argv[5]);
  string table_filename_prefix = table_filename_prefixes[player_to_move - 1];
  string database_path = "data/db/" + table_filename_prefix + '_' + to_string(self_blue_count) + '-' + to_string(self_red_count) + '-' + to_string(opponent_blue_count) + '-' + to_string(opponent_red_count) + ".bin";
  
  unique_ptr<ZDD> zdd = make_unique<ZDD>(self_blue_count, self_red_count, opponent_blue_count + opponent_red_count);
  Position position;

  // Sample with replacement; seed the generator from the current clock.
  unsigned long long int sampled_configuration_ids[100];
  mt19937_64 random_engine(high_resolution_clock::now().time_since_epoch().count());
  uniform_int_distribution<unsigned long long int> configuration_id_distribution(0, placement_count[self_blue_count][self_red_count][opponent_blue_count + opponent_red_count] - 1);
  for(int sample_index = 0; sample_index < 100; sample_index++) {
    sampled_configuration_ids[sample_index] = configuration_id_distribution(random_engine);
  }

  std::ifstream database_file(database_path, std::ios::binary);
  if (!database_file) {
    cerr << "[error] Failed to open database: " << database_path << endl;
    return 1;
  }

  for(int sample_index = 0; sample_index < 100; sample_index++) {
    unsigned long long int configuration_id = sampled_configuration_ids[sample_index];
    
    // Skip the 8-byte header; each byte stores four 2-bit entries.
    unsigned long long int entry_byte_offset = 8 + configuration_id / 4;
    database_file.seekg(static_cast<std::streamoff>(entry_byte_offset));

    char packed_labels;
    if(!database_file.read(&packed_labels, 1)) {
      cerr << "[error] Failed to read database: " << database_path
	   << ", configuration_id: " << configuration_id << endl;
      return 1;
    }

    // Extract the 2-bit label; entries are packed from the least significant bits.
    unsigned int label_code = (static_cast<unsigned char>(packed_labels) >> (2U * (configuration_id % 4))) & 3U;
    
    cout << "[lookup] configuration_id: " << configuration_id << endl;

    position.load_configuration(configuration_id, *zdd, self_blue_count, self_red_count, opponent_blue_count, opponent_red_count);
    position.print();
    
    cout << "[lookup] label_code: " << label_code;
    
    if(label_code == v_unknown) cout << ", label: unknown" << endl;
    else if(label_code == v_win) cout << ", label: forced win (FW)" << endl;
    else if(label_code == v_loss) cout << ", label: forced loss (FL)" << endl;
    else if(label_code == v_can_lose) cout << ", label：can lose (CL)" << endl;
    else {
      cerr << "\n[error] Invalid label code: " << label_code << endl;
      return 1;
    }
  }

  database_file.close();

  return 0;
}
