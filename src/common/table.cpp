#include "table.hpp"

// Sequential reader for an 8-byte header and packed label entries.
class TableReader {
private:
    unsigned char m_buffer;
    int m_buffered_entry_count;
    fstream m_stream;

public:
  TableReader() = delete;
  TableReader(int iteration, const string &database_path, size_t configuration_count,
	      TableReadOutput read_output) noexcept;
  ~TableReader() noexcept {
    assert(m_buffered_entry_count == 0);
    if(!m_stream) {
      cerr << "Read Error" << endl;
      terminate();
    }
    m_stream.close();
  }

  // Consume low-order bits first, matching TableWriter's on-disk order.
  unsigned int read(size_t bits_per_entry) noexcept {
    assert(8 % bits_per_entry == 0);
    if(m_buffered_entry_count == 0) {
      if(!m_stream.read((char*)&m_buffer, 1U)) {
	cerr << "Error: failed to read byte from file\n";
	terminate();
      }
      m_buffered_entry_count = 8 / bits_per_entry; 
    }

    unsigned int mask = (1U << bits_per_entry) - 1U;
    unsigned int label = static_cast<unsigned int>(m_buffer) & mask;
    
    m_buffer >>=  bits_per_entry;
    m_buffered_entry_count--;
    
    return label;
  }    
};

TableReader::TableReader(int iteration, const string &database_path, size_t configuration_count,
			 TableReadOutput read_output) noexcept : m_buffer(0), m_buffered_entry_count(0), m_stream(database_path, fstream::in | fstream::binary) {
  assert(database_path.size() > 0 && database_path.size() < 255);
  // Header: reserved byte. iteration byte, then a 6-byte little-endian entry count.
  unsigned char header[8] = {};
  size_t header_configuration_count = 0ULL;
  // Reject incomplete headers before inspecting their contents.
  if(!m_stream.read(reinterpret_cast<char*>(header), 8U)) {
    cerr << "[error] Failed to read database header: " << database_path << endl;
    terminate();
  }

  if(read_output == TableReadOutput::verbose) {
    for(int i = 0; i < 8; i++) {
      cout << "header[" << i << "]: " << (int)header[i] << endl;
    }
  }

  // Decode the entry count independently of the machine byte order.
  for(int i = 5; i >= 0; i--) {
    header_configuration_count *= 256ULL;    
    header_configuration_count += header[i+2];
  }

  if(header[0] != 0 || (header[1] != iteration && iteration != 0)) {
    cerr << "Header Error" << endl;
    terminate();
  }

  if(header_configuration_count != configuration_count) {
    cerr << "Size Error " << endl;
    terminate();
  }
}

Table::Table(int iteration, const char* database_path, size_t bits_per_entry,
	     unsigned long long int configuration_count, TableReadOutput read_output) noexcept
  : m_bits_per_entry(bits_per_entry), m_entries_per_word(64 / m_bits_per_entry) {
  m_word_count = (configuration_count + m_entries_per_word - 1ULL) / m_entries_per_word;

  assert(database_path && database_path[0] != '\0');
  if(read_output == TableReadOutput::verbose) {
    cout << "[table] Reading database: " << database_path << endl;
  }

  // set() preserves neighboring bits, so initialize every word before filling entries.
  m_table = new uint64_t[m_word_count]();

  // Preserve analysis behavior: an unavailable input starts as an unknown table.
  fstream database_file(database_path, fstream::in | fstream::binary);
  if(!database_file) {
    if(read_output == TableReadOutput::verbose) {
      cout << "[table] Database unavailable; initializing unknown labels: " << database_path << endl;
    }
    // Words were value-initialized above; no file entries need to be read.
  } else {
    {
      unsigned long long int win_count = 0, loss_count = 0, unknown_count = 0, last_unknown_id = 0, can_lose_count = 0;

      TableReader reader(iteration, database_path, configuration_count, read_output);
      if(m_bits_per_entry == 8) {
	for(unsigned long long int i = 0; i < configuration_count; i++) set(i, reader.read(m_bits_per_entry));
      } else {
	for(unsigned long long int i = 0; i < configuration_count; i++) {
	  unsigned int value = reader.read(m_bits_per_entry);
	  
	  if(value == v_win) {
	    win_count++;
	  } else if(value == v_loss) {
	    loss_count++;
	  } else if(value == v_can_lose) {
	    can_lose_count++;
	  } else {
	    unknown_count++;
	    last_unknown_id = i;
	  }
	  set(i, value);
	}

	if(read_output == TableReadOutput::verbose) {
	  cout << "[table] Loaded win labels: " << win_count << endl;
	  cout << "[table] Loaded loss labels: " << loss_count << endl;
	  cout << "[table] Loaded can-lose labels: " << can_lose_count << endl;
	  cout << "[table] Loaded unknown labels: " << unknown_count << endl;

	  if(unknown_count > 0) {
	    cout << "[table] last unknown configuration ID: " << last_unknown_id << endl;
	  }
	}
      }
    }
  }
  database_file.close();
}
