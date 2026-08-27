#ifndef INCLUDE_TABLE
#define INCLUDE_TABLE
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include "zdd_geister.hpp"

using std::fstream;
using std::string;
using std::size_t;
using std::cout;
using std::endl;
using std::cerr;
using std::terminate;

enum { v_unknown = 0, v_win = 1, v_lose = 2, v_can_lose = 3};

class Table {
private:
  uint64_t *m_table;   // Table storing the game results for all configurations
  unsigned long long int m_table_size;
  size_t m_bits_per_entry;
  size_t m_entries_per_word;

public:
  Table() noexcept : m_table(nullptr), m_table_size(0), m_bits_per_entry(0), m_entries_per_word(0) {}
  Table(int iter, const char* read_file_name, size_t bits_per_entry, unsigned long long int placement_size) noexcept;
  ~Table() noexcept { delete [] m_table; }

  //引数で与えたid番のw,l,unkを得る
  unsigned int get(unsigned long long int id) const noexcept {
    unsigned long long int id64 = id / m_entries_per_word;
    unsigned long long int id1 = (id % m_entries_per_word) * m_bits_per_entry;
    if(id64 >= m_table_size) {
      cout << id64 << " : " << m_table_size << endl;
      assert(false);
    }
    // assert(id64 < m_table_size);
    unsigned int v = (unsigned int)((1U << m_bits_per_entry) - 1U) & (unsigned int)(m_table[id64] >> (64ULL - id1 - m_bits_per_entry));
    return v;
  }
  
  //表のid番のところにu2(w,l,unk)をセットする関数
  void set(unsigned long long int id, unsigned int entry) noexcept {
    unsigned long long int id64 = id / m_entries_per_word;
    unsigned long long int id1 = (id % m_entries_per_word) * m_bits_per_entry;
    unsigned long long int mask = (unsigned long long int)((1U << m_bits_per_entry) - 1U) << (64ULL - id1 - m_bits_per_entry);
    assert(id64 < m_table_size);
    unsigned long long int t = m_table[id64] & ~mask;
    m_table[id64] = t | ((unsigned long long int)entry << (64ULL - id1 - m_bits_per_entry));
  }
};

//1繰り返しの最後に表(2bit)を全部書き出すクラス
class OutTable {
private:
  unsigned char m_buffer;
  int m_num_keep;
  fstream m_ofs;
  size_t m_bits_per_entry;
  unsigned long long int m_nwin, m_nlose, m_ncan_lose, m_nunknown;

public:
  OutTable() = delete;
  OutTable(int iter, const string &s, size_t num, size_t bits_per_entry) noexcept
    : m_buffer(0), m_num_keep(0), m_ofs(s, fstream::out | fstream::binary | fstream::trunc),
      m_bits_per_entry(bits_per_entry), m_nwin(0), m_nlose(0), m_ncan_lose(0), m_nunknown(0) {
    assert(s.size() > 0 && s.size() < 255);
    std::cout << "write" << endl;
    unsigned char header[8] = {0};
    header[0] = 0;
    header[1] = (unsigned char)iter;
    for(int i = 2; i < 8; i++){
      header[i] = num % 256U;
      num /= 256U;
    }
    // output header[]
    m_ofs.write((char *)header, 8U);
  }
  
  ~OutTable() noexcept {
    // output keeping data in m_buffer;
    assert(m_num_keep == 0);
    if (! m_ofs) {
      std::cerr << "Write Error" << std::endl;
      std::terminate();
    }
    m_ofs.close();
  }
  //表の各番地に書き込んでいく(bitesがw,l,unk)
  void write(unsigned int entry) noexcept {
    if(entry == v_win) m_nwin++;
    else if(entry == v_lose) m_nlose++;
    else if(entry == v_can_lose) m_ncan_lose++;
    else m_nunknown++;
    
    // put bites to m_buffer
    m_buffer |= static_cast<unsigned char>(entry << (m_num_keep * m_bits_per_entry));
    if (++m_num_keep < static_cast<int>(8 / m_bits_per_entry)) return;
    m_ofs.write((char*)&m_buffer, 1U);
    m_num_keep = 0;
    m_buffer = 0U;
  } 

  void flush() noexcept {
    while(m_num_keep != 0) {
      write(0);
    }
  }

  void outinfo() const noexcept {
    cout << "nwin  = " << m_nwin << endl;
    cout << "nlose = " << m_nlose << endl;
    cout << "ncan_lose = " << m_ncan_lose << endl;
    cout << "nunknown = " << m_nunknown << endl;
    cout << endl;
  }
};

#endif
