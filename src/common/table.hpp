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

enum { v_unknown = 0, v_win = 1, v_loss = 2, v_can_lose = 3 };

// Control informational output while loading; errors are always reported to stderr.
enum class TableReadOutput { verbose, silent };

// Packed in-memory outcome labels, indexed by configuration ID.
class Table {
private:
  uint64_t *m_table;
  unsigned long long int m_word_count;
  size_t m_bits_per_entry;
  size_t m_entries_per_word;

public:
  Table() noexcept : m_table(nullptr), m_word_count(0), m_bits_per_entry(0), m_entries_per_word(0) {}
  Table(int iteration, const char* database_path, size_t bits_per_entry, unsigned long long int configuration_count,
	TableReadOutput read_output = TableReadOutput::verbose) noexcept;
  ~Table() noexcept { delete [] m_table; }

  // Return the label at a valid configuration ID.
  unsigned int get(unsigned long long int id) const noexcept {
    unsigned long long int word_index = id / m_entries_per_word;
    unsigned long long int bit_offset = (id % m_entries_per_word) * m_bits_per_entry;
    if(word_index >= m_word_count) {
      cout << word_index << " : " << m_word_count << endl;
      assert(false);
    }
    
    unsigned int label = (unsigned int)((1U << m_bits_per_entry) - 1U) & (unsigned int)(m_table[word_index] >> (64ULL - bit_offset - m_bits_per_entry));
    return label;
  }
  
  // Replace one packed label without changing adjacent entries.
  void set(unsigned long long int id, unsigned int entry) noexcept {
    unsigned long long int word_index = id / m_entries_per_word;
    unsigned long long int bit_offset = (id % m_entries_per_word) * m_bits_per_entry;
    unsigned long long int mask = (unsigned long long int)((1U << m_bits_per_entry) - 1U) << (64ULL - bit_offset - m_bits_per_entry);
    assert(word_index < m_word_count);
    unsigned long long int remaining_bits = m_table[word_index] & ~mask;
    m_table[word_index] = remaining_bits | ((unsigned long long int)entry << (64ULL - bit_offset - m_bits_per_entry));
  }
};

// Write an 8-byte header and labels packed from the least significant bits.
class TableWriter {
private:
  unsigned char m_buffer;
  int m_buffered_entry_count;
  fstream m_stream;
  size_t m_bits_per_entry;
  unsigned long long int m_win_count, m_loss_count, m_can_lose_count, m_unknown_count;

public:
  TableWriter() = delete;
  TableWriter(int iteration, const string &database_path, size_t configuration_count, size_t bits_per_entry) noexcept
    : m_buffer(0), m_buffered_entry_count(0), m_stream(database_path, fstream::out | fstream::binary | fstream::trunc),
      m_bits_per_entry(bits_per_entry), m_win_count(0), m_loss_count(0), m_can_lose_count(0), m_unknown_count(0) {
    assert(database_path.size() > 0 && database_path.size() < 255);
    cout << "[table] writing database: " << database_path << endl;
    // Header: reserved byte, iteration byte, then a 6-byte little-endian entry count.
    unsigned char header[8] = {0};
    header[0] = 0;
    header[1] = (unsigned char)iteration;
    for(int i = 2; i < 8; i++){
      header[i] = configuration_count % 256U;
      configuration_count /= 256U;
    }

    m_stream.write((char *)header, 8U);
  }
  
  ~TableWriter() noexcept {
    assert(m_buffered_entry_count == 0);
    if(!m_stream) {
      cerr << "Write Error" << endl;
      terminate();
    }
    m_stream.close();
  }
  
  // Append one label and emit the buffer when it contains a complete byte.
  void write(unsigned int entry) noexcept {
    if(entry == v_win) m_win_count++;
    else if(entry == v_loss) m_loss_count++;
    else if(entry == v_can_lose) m_can_lose_count++;
    else m_unknown_count++;
    
    m_buffer |= static_cast<unsigned char>(entry << (m_buffered_entry_count * m_bits_per_entry));
    if(++m_buffered_entry_count < static_cast<int>(8 / m_bits_per_entry)) return;
    m_stream.write((char*)&m_buffer, 1U);
    m_buffered_entry_count = 0;
    m_buffer = 0U;
  }

  // Pad the final byte with unknown labels; preserve the existing statistics behavior.
  void flush() noexcept {
    while(m_buffered_entry_count != 0) {
      write(0);
    }
  }

  // Print written label counts, including any padding added by flush.
  void print_statistics() const noexcept {
    cout << "[table] Written win labels: " << m_win_count << endl;
    cout << "[table] Written loss labels: " << m_loss_count << endl;
    cout << "[table] Written can-lose labels: " << m_can_lose_count << endl;
    cout << "[table] Written unknown labels: " << m_unknown_count << endl;
  }
};

#endif
