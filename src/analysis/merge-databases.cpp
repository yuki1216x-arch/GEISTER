// 2つのデータベースを組み合わせて各要素の数を表示する
#include <cassert>
#include <iostream>
#include <thread>
#include <fstream>
#include "posi.hpp"

using namespace std;

enum { v_unknown = 0, v_win = 1, v_lose = 2, v_can_lose = 3};

ZDD& ZDD::get(int iter, int i, int j, int k, int l) noexcept {
    static ZDD inst1_1_2(1, 1, 2), inst1_1_3(1, 1, 3), inst2_1_2(2, 1, 2), inst1_2_2(1, 2, 2), inst1_2_3(1, 2, 3);
    if(i == 1 && j == 1 && k == 1 && l == 1) return inst1_1_2;
    else if(i == 1 && j == 1 && k == 2 && l == 1) return inst1_1_3;
    else if(i == 1 && j == 1 && k == 1 && l == 2) return inst1_1_3;
    else if(i == 2 && j == 1 && k == 1 && l == 1) return inst2_1_2;
    else if(i == 1 && j == 2 && k == 1 && l == 1) return inst1_2_2;
    else if(i == 1 && j == 2 && k == 1 && l == 2) return inst1_2_3;
    else std::terminate();
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
    InTable(const string &s, std::size_t num) noexcept : m_buffer(0), m_num_keep(0), m_ofs(s, fstream::in | fstream::binary) {
        unsigned char header[8];    //header[0]: 0で固定, header[1]: iterの回数, 以降: 配置数
        std::size_t num_check = 0ULL;
        for(int i = 0; i < 8; i++) m_ofs.read((char*)header+i, 1U); //各表(2bit)の前のヘッダーを読み込む(ヘッダーは個人的なやつ)
        
        for(int i = 0; i < 8; i++) {
            std::cout << "header[" << i << "]: " << (int)header[i] << endl;
        }

        //headerに記憶されている配置数を取得(256進数)
        for(int i = 5; i >= 0; i--){
            num_check *= 256ULL;    
            num_check += header[i+2];
        }
        
        //配置数の確認(header[2-7])
        if(num_check != num) {
            cerr << "Size Error" << endl;
            std::terminate();
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
    int iterations; //繰り返し回数

public:
    //iter: 今の反復回数, read_file_name: 読み込むファイルの名前, write_file_name: 書き込み先のファイルの名前
    Table(string read_file_name, unsigned long long int table_size) noexcept {
        unsigned long long int table_size64_2 = (table_size + 31ULL) / 32ULL;
        m_table = new uint64_t [table_size64_2];
        std::cout << "read_file_name: " << read_file_name << endl;
        fstream read_file (read_file_name, fstream::in | fstream::binary);
        if(!read_file) {    //readファイルがなかった場合
            std::cout << "no such file: " << read_file_name << endl;
            std::terminate();
        }
        InTable in_table(read_file_name, table_size); //ここのInTableでエラー
        for(unsigned long long int i = 0; i < table_size; i++) set(i, in_table.read());
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
    int get_iterations() const noexcept {
        return iterations;
    }
};

// 表(8bit)を読み込むクラス
class InTable8 {
    private:
        unsigned char m_buffer;
        int m_num_keep;
        fstream m_ofs8;
    
    public:
        InTable8() = delete; 
        //iter: 表を読み込むときに、今週目か, s: ファイル名, num: 配置数
        InTable8(const string &s, std::size_t num) noexcept : m_buffer(0), m_num_keep(0), m_ofs8(s, fstream::in | fstream::binary) {
            if(!m_ofs8) {    //readファイルがなかった場合
                cerr << "no such file: " << s << endl;
                terminate();
            }
        }
        ~InTable8() noexcept {
            // assert(m_num_keep == 0);
            if (! m_ofs8) {
                std::cerr << "Read Error8" << std::endl;
                std::terminate();
            }
            m_ofs8.close();  //ファイルを閉じる
        }
        //表を読み込む関数(ヘッダ以降)
        //返り値:あるidでの値(w/l/unk/new)
        unsigned int read() noexcept {
            m_ofs8.read((char*)&m_buffer, 1U);
            return m_buffer;
            // if(m_num_keep == 0) {
            //     m_ofs8.read((char*)&m_buffer, 1U);   
            //     m_num_keep = 1; 
            // }
            // unsigned int val = m_buffer & 255U;   //今のbuffer(アドレス)の11(3U)と＆を取って、値を読み取る(val)
            // m_buffer =  m_buffer >> 8U; //2bit分アドレスを進める
            // m_num_keep--;
            // return val;
        }    
    };
    
    //表(8bit)をメモリ上に表を持つためのクラス
    class Table8 {
    private:
        uint64_t *m_table;  //これで配置数分*2
        int iterations; //繰り返し回数
    
    public:
        //iter: 今の反復回数, read_file_name: 読み込むファイルの名前, write_file_name: 書き込み先のファイルの名前
        Table8(string read_file_name, unsigned long long int table_size) noexcept {
            unsigned long long int table_size64_8 = (table_size + 7ULL) / 8ULL;
            m_table = new uint64_t[table_size64_8];
            cout << "read_file_name: " << read_file_name << endl;
            fstream read_file (read_file_name, fstream::in | fstream::binary);
            if(!read_file) {    //readファイルがなかった場合
                cerr << "no such file: " << read_file_name << endl;
                terminate();
            }
            cout << "aaaa" << endl;
            InTable8 in_table(read_file_name, table_size); //ここのInTableでエラー
            cout << "bbbb" << endl;
            for(unsigned long long int i = 0; i < table_size; i++) {
                unsigned int v = in_table.read();   //in_Tableからidを一つずつ読み込んでいき、その値をvに代入
                if(i < 10) cout << "v: " << v << endl;
                set(i, v);   
            }
            read_file.close();
        }
        ~Table8() noexcept { delete [] m_table; }
    
        //引数で与えたid番のw,l,unkを得る
        unsigned int get(unsigned long long int id2) const noexcept {
            unsigned long long int id64 = id2 / 8ULL;
            unsigned long long int id1 = (id2 % 8ULL) * 8ULL;
            unsigned int v = 255U & (unsigned int)(m_table[id64] >> (64ULL-id1-8ULL));
            return v;
        }
        //表(2bit)のid2番のところにu2(w,l,unk)をセットする関数
        void set(unsigned long long int id2, unsigned int u2) noexcept {
            unsigned long long int id64 = id2 / 8ULL;
            unsigned long long int id1 = (id2 % 8ULL) * 8ULL;
            unsigned long long int mask = 255ULL << (64ULL-id1-8ULL);
            unsigned long long int t = m_table[id64] & ~mask; // いらない説
            m_table[id64] = t | ((unsigned long long int)u2 << (64ULL-id1-8ULL));
        }
        int get_iterations() const noexcept {
            return iterations;
        }
    };

int main(int argc, char *argv[]) {
    std::string read_file_name1 = argv[1], read_file_name2 = argv[2];
    // std::string write_file_name1 = argv[4];
    ZDD::get(1, 1, 2, 1, 2).out_info();
    unsigned long long int table_size = atoi(argv[3]);
    Table table1(read_file_name1, table_size);
    Table table2(read_file_name2, table_size);
    // Table8 table_db(write_file_name1, table_size);
    unsigned long long int win_win = 0, win_nowin = 0, nolose_win = 0, nolose_nowin = 0, 
                           canlose_win = 0, canlose_nowin = 0, lose_win = 0, lose_nowin = 0;
    for(unsigned long long int i = 0; i < table_size; i++) {
        unsigned int value1 = table1.get(i), value2 = table2.get(i);
        if(value1 == v_win && value2 == v_win) win_win++;
        else if(value1 == v_win && value2 == v_unknown) {
            // if(table_db.get(i) == 3) {
            //     POSITION posi(1, 1, 2, 1, 1, i);
            //     std::cout << posi.array_to_fen() << " : " << i << endl;
            // }
            win_nowin++;
        }
        else if(value1 == v_unknown && value2 == v_win) nolose_win++;
        else if(value1 == v_unknown && value2 == v_unknown) nolose_nowin++;
        else if(value1 == v_can_lose && value2 == v_win) {
            canlose_win++;
            // if(canlose_win % 1000000 == 0) {
            //     POSITION posi(1, 1, 2, 1, 2, i);
            //     posi.print_board();
            // }
        }
        else if(value1 == v_can_lose && value2 == v_unknown) canlose_nowin++;
        else if(value1 == v_lose && value2 == v_win) {
            lose_win++;
            // if(table_db.get(i) == 1) {
            //     POSITION posi(1, 1, 1, 1, 1, i);
            //     std::cout << posi.array_to_fen() << endl;
            // }
        }
        else if(value1 == v_lose && value2 == v_unknown) lose_nowin++;
        else {
            std::terminate();
        }
    }

    std::cout << "forced win & can win     =  " << win_win << endl;
    std::cout << "forced win & cannot win  =  " << win_nowin << endl;
    std::cout << "cannot lose & can win    =  " << nolose_win << endl;
    std::cout << "cannot lose & cannot win =  " << nolose_nowin << endl;
    std::cout << "can lose & can win       =  " << canlose_win << endl;
    std::cout << "can lose & cannot win    =  " << canlose_nowin << endl;
    std::cout << "forced lose & can win    =  " << lose_win << endl;
    std::cout << "forced lose & cannot win =  " << lose_nowin << endl;
    std::cout << "finish!!" << endl;

    return 0;

    // ----------------------------------------------------------------------------------
    // 一連の流れをやってみる
    // POSITION posi(2,1,2,1,1,350);
    // Combination combination[64];
    // int nstate = posi.gen_belief_state(combination);
    // cout << "belief state size: " << nstate << endl;
    // char input;

    // for(int stateid = 0; stateid < nstate; stateid++) {
    //     cout << "belief state number: " << stateid << endl;
    //     posi.do_belief_state(combination[stateid]);
    //     posi.print_board();
    //     Action actions[64];
    //     int naction = posi.gen_actions(actions);
    //     cout << "child action size: " << naction << endl;
    //     for(int actionid = 0; actionid < naction; actionid++) {
    //         cout << "child action number: " << actionid << endl;
    //         posi.do_action(actions[actionid]);
    //         if(posi.is_end() == 0) cout << "this configuration id: " << posi.compute_id() << endl;
    //         else cout << "this configuration is terminated." << endl;
    //         posi.print_board();
    //         posi.do_flip();
    //         if(posi.is_end() == 0) cout << "flip configuration id: " << posi.compute_id() << endl;
    //         else cout << "flip configuration is terminated." << endl;
    //         posi.print_board();
    //         posi.undo_flip();
    //         posi.undo_action();
    //         cin >> input;
    //     }
    //     posi.undo_belief_state();
    // }
    
    // return 0;

    // ----------------------------------------------------------------------------------
    // ある配置の勝敗を調べる
    // std::string read_file_name1 = "./table/table_cw1-2-1-1_p2.bin";
    // // std::string read_file_name1 = "./table/table1-1-1-2_p1.bin";
    // ZDD::get(1, 1, 2, 1, 1).out_info();
    // Table table1(read_file_name1, 11309760);

    // string fen = "UU1U1b/r5/6/6/6/6";
    // string turn = "1";
    // POSITION posi = POSITION();
    // cout << "aaa" << endl;
    // posi.fen_to_array(fen, turn, 1, 1, 2, 1);
    // posi.print_board();

    // unsigned long long int id = posi.compute_id();
    // cout << id << endl;
    
    // POSITION posi(1,1,2,1,1,350);
    // posi.print_board();

    // unsigned int value = table1.get(350);
    // if(value == v_win) cout << "必勝" << endl;
    // else if(value == v_unknown) cout << "負けなし" << endl;
    // else if(value == v_can_lose) cout << "負けあり" << endl;
    // else if(value == v_lose) cout << "必敗" << endl;
    // else cout << "ん？妙だな..." << endl;
    
    // return 0;

    // ---------------------------------------------------------------------------------------------
    // あるfen形式のidを調べる
    // string fen = "6/6/Ur1b2/1U4/6/6";
    // string turn = "1";
    // POSITION posi = POSITION();
    // cout << "aaa" << endl;
    // posi.fen_to_array(fen, turn, 1, 1, 1, 1);
    
    // unsigned long long int id = posi.compute_id();
    // cout << id << endl;
    // cout << table1.get(id) << ':' << table2.get(id) << ':' << table_db.get(id) << endl;
    // return 0;
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
