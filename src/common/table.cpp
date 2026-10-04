#include "table.hpp"

// Class for reading tables
class InTable {
private:
    unsigned char m_buffer;
    int m_num_keep;
    fstream m_ofs;

public:
  InTable() = delete;
  InTable(int iter, const string &s, size_t num) noexcept;
  ~InTable() noexcept {
    assert(m_num_keep == 0);
    if (! m_ofs) {
      cerr << "Read Error" << endl;
      terminate();
    }
    m_ofs.close();  // close the file
  }
  
  unsigned int read(size_t bits_per_entry) noexcept {
    assert(8 % bits_per_entry == 0);
    if(m_num_keep == 0) {
      if(!m_ofs.read((char*)&m_buffer, 1U)) {
	cerr << "Error: failed to read byte from file\n";
	terminate();
      }
      m_num_keep = 8 / bits_per_entry; 
    }

    unsigned int mask = (1U << bits_per_entry) - 1U;
    unsigned int val = static_cast<unsigned int>(m_buffer) & mask;   // read the value (val) by masking the required bits from the current buffer address
    
    m_buffer >>=  bits_per_entry; // advance the address by the number of bits read
    m_num_keep--;
    
    return val;
  }    
};

InTable::InTable(int iter, const string &s, size_t num) noexcept : m_buffer(0), m_num_keep(0), m_ofs(s, fstream::in | fstream::binary) {
  assert(s.size() > 0 && s.size() < 255);
  unsigned char header[8];   // header[0]: always 0, header[1]: number of iterations, remaining entries: number of configurations
  size_t num_check = 0ULL;
  for(int i = 0; i < 8; i++) m_ofs.read((char*)header+i, 1U); // read the header of each table

  for(int i = 0; i < 8; i++) {
    cout << "header[" << i << "]: " << (int)header[i] << endl;
  }
  
  // retrieve the number of configurations stored in the header(base-256 number)
  for(int i = 5; i >= 0; i--) {
    num_check *= 256ULL;    
    num_check += header[i+2];
  }
  // validate the values of header[0], and header[1]
  if(header[0] != 0 || (header[1] != iter && iter != 0)) {
    cerr << "Header Error" << endl;
    terminate();
  }
  // verify the number of configurations in the header[2-7]
  if(num_check != num) {
    cerr << "Size Error " << endl;
    terminate();
  }
}

Table::Table(int iter, const char* read_file_name, size_t bits_per_entry, unsigned long long int placement_size) noexcept
  : m_bits_per_entry(bits_per_entry), m_entries_per_word(64 / m_bits_per_entry) {
  m_table_size = (placement_size + m_entries_per_word - 1ULL) / m_entries_per_word;

  assert(read_file_name && read_file_name[0] != '\0');
  cout << "read_file_name: " << read_file_name << endl;

  m_table = new uint64_t [m_table_size];
  
  fstream read_file (read_file_name, fstream::in | fstream::binary);
  if(!read_file) {    // if the input file does not exist
    cout << "no file: " << endl;
    for(size_t tableid = 0; tableid < m_table_size; tableid++) m_table[tableid] = 0ULL; // initialize all tables to zero
  } else {
    {
      unsigned long long int nwin = 0, nlose = 0, nunknown = 0, lunknown = 0, ncanlose = 0;

      cout << "aaaa" << endl;
      InTable in_table(iter, read_file_name, placement_size);
      cout << "bbbb" << endl;
      if(m_bits_per_entry == 8) {
	for(unsigned long long int i = 0; i < placement_size; i++) set(i, in_table.read(m_bits_per_entry));
      } else {
	for(unsigned long long int i = 0; i < placement_size; i++) {
	  unsigned int value = in_table.read(m_bits_per_entry);   // read each ID from in_table and assign its value to v
	  
	  if(value == v_win) {
	    nwin++;
	  } else if(value == v_lose) {
	    nlose++;
	  } else if(value == v_can_lose) {
	    ncanlose++;
	  } else {
	    nunknown++;
	    lunknown = i;
	  }
	  set(i, value);
	}

	cout << "before nwin = " << nwin << endl;
	cout << "before nlose = " << nlose << endl;
	cout << "before ncanlose = " << ncanlose << endl;
	cout << "before nunknown = " << nunknown << endl;
	cout << "last unknown = " << lunknown << endl;
      }
    }
  }
  read_file.close();
}
