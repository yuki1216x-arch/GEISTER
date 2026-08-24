#ifndef DATABASE_HPP
#define DATABASE_HPP
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include "zdd.hpp"

using namespace std;

enum { v_unknown = 0, v_win = 1, v_lose = 2, v_can_lose = 3};

// 必要な表(4bit)を読み込むクラス
class DInTable {
private:
    unsigned char m_buffer;
    int m_num_keep;
    fstream m_ofs;

public:
    DInTable() = delete; 
    //iter: 表を読み込むときに、今週目か, s: ファイル名, num: 配置数
    DInTable(const string &s, std::size_t num) noexcept : m_buffer(0), m_num_keep(0), m_ofs(s, fstream::in | fstream::binary) {
        unsigned char header[8];    //header[0]: 0で固定, header[1]: iterの回数, 以降: 配置数
        std::size_t num_check = 0ULL;
        for(int i = 0; i < 8; i++) m_ofs.read((char*)header+i, 1U); //各表(2bit)の前のヘッダーを読み込む(ヘッダーは個人的なやつ)

        //headerに記憶されている配置数を取得(256進数)
        for(int i = 5; i >= 0; i--){
            num_check *= 256ULL;    
            num_check += header[i+2];
            // cout << num_check << endl;
        }

        //配置数の確認(header[2-7])
        if(num_check != num) {
            std::cout << num_check << "a" << num << endl;
            cerr << "Size Error " << s << endl;
            terminate();
        }
    }
    ~DInTable() noexcept {
        // assert(m_num_keep == 0);
        if (! m_ofs) {
            std::cerr << "Read Error" << std::endl;
            std::terminate();
        }
        m_ofs.close();  //ファイルを閉じる
    }
    //表を読み込む関数(ヘッダ以降)
    //返り値:あるidでの値(w/l/unk/new)
    unsigned int read() noexcept {
        if(m_num_keep == 0) {
            m_ofs.read((char*)&m_buffer, 1U);   
            m_num_keep = 4; 
        }
        unsigned int val = m_buffer & 3U;   //今のbuffer(アドレス)の11(3U)と＆を取って、値を読み取る(val)
        m_buffer =  m_buffer >> 2U; //2bit分アドレスを進める
        m_num_keep--;
        return val;
    }    
};

//表(4bit)をメモリ上に表を持つためのクラス
class DTable {
private:
    uint64_t *m_table;  //これで配置数分*2
    size_t table_size;  //

public:
    DTable() noexcept : m_table(nullptr), table_size(0) {}
    //iter: 今の反復回数, read_file_name: 読み込むファイルの名前, write_file_name: 書き込み先のファイルの名前
    DTable(string read_file_name, unsigned long long int size) noexcept : table_size(size) {
        unsigned long long int table_size64 = (table_size + 31ULL) / 32ULL;
        m_table = new uint64_t[table_size64];
        
        cout << "read_file_name: " << read_file_name << endl;
        
        fstream read_file (read_file_name, fstream::in | fstream::binary);
        if(!read_file) {    //readファイルがなかった場合
            cout << "no such file: " << read_file_name << endl;
            m_table = nullptr;
            table_size = 0;
        } else {
            DInTable in_table(read_file_name, table_size); //ここのInTableでエラー
            for(unsigned long long int i = 0; i < table_size; i++) set(i, in_table.read());   //in_Tableからidを一つずつ読み込んでいき、その値をvに代入
            cout << read_file_name << " could be read!!" << endl;
            read_file.close();
        }
    }
    ~DTable() noexcept { delete [] m_table; }

    size_t get_size() const noexcept {
        return table_size;
    }

    //引数で与えたid番のw,l,unkを得る
    unsigned int get(unsigned long long int id2) noexcept {
        // cout << table_size << endl;
        // if(m_table == nullptr) return 4U;
        assert(m_table != nullptr);
        assert(id2 < table_size);
        unsigned long long int id64 = id2 / 32ULL;
        unsigned long long int id1 = (id2 % 32ULL) * 2ULL;
        unsigned int v = 3U & (unsigned int)(m_table[id64] >> (64ULL-id1-2ULL));
        // cout << "ccc" << endl;
        return v;
    }
    //表(2bit)のid2番のところにu2(w,l,unk)をセットする関数
    void set(unsigned long long int id2, unsigned int u2) noexcept {
        unsigned long long int id64 = id2 / 32ULL;
        unsigned long long int id1 = (id2 % 32ULL) * 2ULL;
        unsigned long long int mask = 3ULL << (62ULL-id1);
        assert(id2 < table_size && id64 < (table_size + 31ULL) / 32ULL);
        unsigned long long int t = m_table[id64] & ~mask; // いらない説
        m_table[id64] = t | ((unsigned long long int)u2 << (62ULL-id1));
    }
};

//必要なデータベースを保持するクラス
class Database {
private:
    unsigned int num_b, num_r, num_eb, num_er;
    DTable *dtable, *dtable_cap_b, *dtable_cap_r,
           *dtable_flip, *dtable_flip_cap_b, *dtable_flip_cap_r;
    // unsigned long long int num;
    
public:
    Database() noexcept : dtable(NULL) {}; // コンストラクタを private に置く。
    Database(int iter, int i, int j, int k, int l) noexcept: num_b(i), num_r(j), num_eb(k), num_er(l) {
        if(iter % 2 == 1) {
            string file_name = "./table/table_cw" + to_string(i) + '-' + to_string(j) + '-' + to_string(k) + '-' + to_string(l) + "_p2.bin";
            dtable = new DTable(file_name, ZDD::get(iter, i, j, k, l).get_num());
            string file_name_flip = "./table/table" + to_string(k) + '-' + to_string(l) + '-' + to_string(i) + '-' + to_string(j) + "_p1.bin";
            dtable_flip = new DTable(file_name_flip, ZDD::get(iter, k, l, i, j).get_num());
            if(k < 2) dtable_cap_b = new DTable(), dtable_flip_cap_b = new DTable();
            else {
                string file_name_cap_b = "./table/table_cw" + to_string(i) + '-' + to_string(j) + '-' + to_string(k-1) + '-' + to_string(l) + "_p2.bin";
                string file_name_flip_cap_b = "./table/table" + to_string(k-1) + '-' + to_string(l) + '-' + to_string(i) + '-' + to_string(j) + "_p1.bin";
                dtable_cap_b = new DTable(file_name_cap_b, ZDD::get(iter, i, j, k-1, l).get_num());
                dtable_flip_cap_b = new DTable(file_name_flip_cap_b, ZDD::get(iter, k-1, l, i, j).get_num());
            }
            if(l < 2) dtable_cap_r = new DTable(), dtable_flip_cap_r = new DTable();
            else { 
                string file_name_cap_r = "./table/table_cw" + to_string(i) + '-' + to_string(j) + '-' + to_string(k) + '-' + to_string(l-1) + "_p2.bin";
                string file_name_flip_cap_r = "./table/table" + to_string(k) + '-' + to_string(l-1) + '-' + to_string(i) + '-' + to_string(j) + "_p1.bin";
                dtable_cap_r = new DTable(file_name_cap_r, ZDD::get(iter, i, j, k, l-1).get_num());
                dtable_flip_cap_r = new DTable(file_name_flip_cap_r, ZDD::get(iter, k, l-1, i, j).get_num());
            }
        } else {
            string file_name = "./table/table_cw" + to_string(i) + '-' + to_string(j) + '-' + to_string(k) + '-' + to_string(l) + "_p1.bin";
            dtable = new DTable(file_name, ZDD::get(iter, i, j, k, l).get_num());
            string file_name_flip = "./table/table" + to_string(k) + '-' + to_string(l) + '-' + to_string(i) + '-' + to_string(j) + "_p2.bin";
            dtable_flip = new DTable(file_name_flip, ZDD::get(iter, k, l, i, j).get_num());
            if(i < 2) dtable_cap_b = new DTable(), dtable_flip_cap_b = new DTable();
            else {
                string file_name_cap_b = "./table/table_cw" + to_string(i-1) + '-' + to_string(j) + '-' + to_string(k) + '-' + to_string(l) + "_p1.bin";
                string file_name_flip_cap_b = "./table/table" + to_string(k) + '-' + to_string(l) + '-' + to_string(i-1) + '-' + to_string(j) + "_p2.bin";
                dtable_cap_b = new DTable(file_name_cap_b, ZDD::get(iter, i-1, j, k, l).get_num());
                dtable_flip_cap_b = new DTable(file_name_flip_cap_b, ZDD::get(iter, k, l, i-1, j).get_num());
            }
            if(j < 2) dtable_cap_r = new DTable(), dtable_flip_cap_r = new DTable();
            else {
                string file_name_cap_r = "./table/table_cw" + to_string(i) + '-' + to_string(j-1) + '-' + to_string(k) + '-' + to_string(l) + "_p1.bin";
                string file_name_flip_cap_r = "./table/table" + to_string(k) + '-' + to_string(l) + '-' + to_string(i) + '-' + to_string(j-1) + "_p2.bin";
                dtable_cap_r = new DTable(file_name_cap_r, ZDD::get(iter, i, j-1, k, l).get_num());
                dtable_flip_cap_r = new DTable(file_name_flip_cap_r, ZDD::get(iter, k, l, i, j-1).get_num());
            }
        }
    }; // コンストラクタを private に置く。
    ~Database() noexcept {}; // デストラクタを private に置く。
    void out_info() const noexcept {
        cout << "table_size: " << dtable->get_size() << endl;
    }
    unsigned int get_value(unsigned int capture, unsigned long long int id2) {
        if(capture == 0U) return dtable->get(id2);
        else if(capture == blue) return dtable_cap_b->get(id2);
        else if(capture == red) return dtable_cap_r->get(id2);
        else std::terminate();
    }
    unsigned int get_value_flip(unsigned int capture, unsigned long long int id2) {
        if(capture == 0U) return dtable_flip->get(id2);
        else if(capture == blue) return dtable_flip_cap_b->get(id2);
        else if(capture == red) return dtable_flip_cap_r->get(id2);
        else std::terminate();
    }
    size_t get_size() { return dtable->get_size(); }
};

#endif