#include <deque>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <thread>
#include <map>
#include <stdint.h>
#include <climits>
#include <exception>
#include <fstream>
#include "zdd.hpp"

using namespace std;

unsigned int NUM_B, NUM_R, NUM_EB, NUM_ER;

size_t max_table_size; //求める駒割の配置数
unsigned long long int table_size64_2; //全体の表2bitの方 //(12352692569ULL + 7ULL)
unsigned long long int count_no_match = 0ULL; //2つのルールで値が合わない配置を数える(0であってほしい)
constexpr int nboss = 8;  //並列数

unsigned long long int nwin = 0, nlose = 0, ncan_lose = 0, nunknown = 0;
enum { v_unknown = 0, v_win = 1, v_lose = 2, v_can_lose = 3};

ZDD& ZDD::get(int i, int j, int k, int l) noexcept {
    static ZDD inst(i, j, k+l); // private なコンストラクタを呼び出す。
    assert(i == NUM_B && j == NUM_R && k == NUM_EB && l == NUM_ER);
    return inst;
}

// 表(4bit)を読み込むクラス
class InTable {
private:
    unsigned char m_buffer;
    int m_num_keep;
    fstream m_ofs;

public:
    InTable() = delete; 
    //iter: 表を読み込むときに、今週目か, s: ファイル名, num: 配置数
    InTable(const string &s) noexcept : m_buffer(0), m_num_keep(0), m_ofs(s, fstream::in | fstream::binary) {
        unsigned char header[8];    //header[0]: 0で固定, header[1]: iterの回数, 以降: 配置数
        std::size_t num_check = 0ULL;
        for(int i = 0; i < 8; i++) m_ofs.read((char*)header+i, 1U); //各表(2bit)の前のヘッダーを読み込む(ヘッダーは個人的なやつ)
        
        for(int i = 0; i < 8; i++) {
            cout << "header[" << i << "]: " << (int)header[i] << endl;
        }

        //headerに記憶されている配置数を取得(256進数)
        for(int i = 5; i >= 0; i--){
            num_check *= 256ULL;    
            num_check += header[i+2];
        }
        //header[0],[1]の値を正誤判定(0回目はなしで)
        if(header[0] != 0 || header[1] == 0) {
            cerr << "Header Error" << endl;
            terminate();
        }
        
        //配置数の確認(header[2-7])
        if(num_check != max_table_size) {
            cerr << "Size Error" << endl;
            terminate();
        }
    }
    ~InTable() noexcept {
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
class Table {
private:
    uint64_t *m_table;  //これで配置数分*2

public:
    //iter: 今の反復回数, read_file_name: 読み込むファイルの名前, write_file_name: 書き込み先のファイルの名前
    Table(string read_file_name) noexcept : m_table(new uint64_t [table_size64_2] ){
        cout << "read_file_name: " << read_file_name << endl;
        fstream read_file (read_file_name, fstream::in | fstream::binary);
        if(!read_file) {    //readファイルがなかった場合
            cerr << "no such file" << endl;
            std::terminate();
        }
        cout << "aaaa" << endl;
        InTable in_table(read_file_name); //ここのInTableでエラー
        cout << "bbbb" << endl;
        for(unsigned long long int i = 0; i < max_table_size; i++) {
            unsigned int v = in_table.read();   //in_Tableからidを一つずつ読み込んでいき、その値をvに代入
            if(v == v_win) {
                nwin++;
            } else if(v == v_lose) {
                nlose++;
            } else if(v == v_unknown) {
                nunknown++;
            } else{
                ncan_lose++;
            } 
            set(i, v);   
        }
        cout << "before nwin  =  " << nwin << endl;
        if(ncan_lose != 0ULL) cout << "before ncan_lose  =  " << ncan_lose << endl;
        cout << "before nlose = " << nlose << endl;
        cout << "before nunknown  =  " << nunknown << endl;
        nwin = 0, ncan_lose = 0, nlose = 0, nunknown = 0;
        read_file.close();
    }
    ~Table() noexcept { delete [] m_table; }

    //引数で与えたid番のw,l,unkを得る
    unsigned int get(unsigned long long int id2) const noexcept {
        unsigned long long int id64 = id2 / 32ULL;
        unsigned long long int id1 = (id2 % 32ULL) * 2ULL;
        unsigned int v = 3U & (unsigned int)(m_table[id64] >> (64ULL-id1-2ULL));
        return v;
    }
    //表(2bit)のid2番のところにu2(w,l,unk)をセットする関数
    void set(unsigned long long int id2, unsigned int u2) noexcept {
        unsigned long long int id64 = id2 / 32ULL;
        unsigned long long int id1 = (id2 % 32ULL) * 2ULL;
        unsigned long long int mask = 3ULL << (62ULL-id1);
        unsigned long long int t = m_table[id64] & ~mask; // いらない説
        m_table[id64] = t | ((unsigned long long int)u2 << (62ULL-id1));
    }
};

//勝敗を出力するだけ
void print_value(unsigned int value) {
    if(value == v_win) cout << "win";
    else if(value == v_lose) cout << "lose";
    else if(value == v_can_lose) cout << "can_lose";
    else cout << "unknown";
}

//ボスの方
static void boss() noexcept {
    unsigned long long int id = 0ULL;  //現在の配置番号(これが配置数分まで行けば終了)
    //unsigned long long int test = 0;

    string read_file_name1 = "./table/table" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + ".bin";
    string read_file_name2 = "./table_purple/table" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + ".bin";
    Table table1(read_file_name1); //メモリにtable1を作る(通常ルール)
    Table table2(read_file_name2); //メモリにtable2を作る(紫駒ルール)

    while(true) {// 仕事がなくなるまで繰り返す
        unsigned int value1 = table1.get(id);
        unsigned int value2 = table2.get(id);

        if(value1 != value2 && (value1 != v_can_lose || value2 != v_lose)) {
            cout << "no_match: " << id << "normal: ";
            print_value(value1);
            cout << ", purple: ";
            print_value(value2);
            cout << endl;
            count_no_match++;
        } else {
            //何もしない
        }

        if(id >= max_table_size-1) break;
        id++;
        if(id % 500000000 == 0) cout << id << " positions was searched!" << endl;
    }
}

int main(int argc, char *argv[]) {
    //通常の処理
    //argv[1~4] : (i, j, k, l)
    cout << "id match search" << endl;
    NUM_B = atoi(argv[1]), NUM_R = atoi(argv[2]), NUM_EB = atoi(argv[3]), NUM_ER = atoi(argv[4]);

    ZDD::get(NUM_B, NUM_R, NUM_EB, NUM_ER).out_info();
    max_table_size = ZDD::get(NUM_B, NUM_R, NUM_EB, NUM_ER).get_num();
    table_size64_2 = (max_table_size + 31ULL) / 32ULL;

    thread th_boss([=]{boss();});    //boss側作る

    //終了処理
    th_boss.join();
    
    if(count_no_match == 0ULL) cout << "all value is matched!!" << endl;
    cout << "program is finished!!" << endl;

    return 0;
}

// g++ -O2 -o zdd.exe zdd_bg.cpp -std=c++11
// ./zdd.exe > res.txt &

// g++ -o gened gened.cpp posi.cpp zdd.cpp
// ./gened 0 table.bin db > res_iter0.txt 2>&1 &
// bash batch.sh > res_iter-.txt 2>&1 &

// g++ -o gened gened.cpp posi.cpp zdd.cpp
// bash batch.sh > database?-?-?-?.txt 2>&1 &

// g++ -DUSE_PURPLE -o gened gened.cpp posi.cpp zdd.cpp

// g++ -o gened gened.cpp database.cpp posi.cpp zdd.cpp
// bash batch.sh | tee database?-?-?-?.txt
