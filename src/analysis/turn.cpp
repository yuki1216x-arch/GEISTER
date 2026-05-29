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
#include <mutex>
#include <condition_variable>
#include "zdd.hpp"

#define max_legal_num 1024

using namespace std;

unsigned int NUM_B, NUM_R, NUM_EB, NUM_ER;

enum { v_unknown = 0, v_win = 1, v_lose = 2, v_can_lose = 3};
unsigned int NOW_SEARCH = v_win;

size_t max_table_size; //求める駒割の配置数
unsigned long long int table_size64_2, table_size64_8; //全体の表2bitの方 //(12352692569ULL + 7ULL)
constexpr int nworker = 7;  //並列数
constexpr int deq_input_size = 1024;    
constexpr int deq_output_size = 256;

condition_variable cv_boss;     //condition_variableもポジックススレッドの排他制御の一つ
condition_variable cv_worker;   
mutex mtx;  
//ポジックススレッディング
//ポジックス-インターフェースの名前
//ここではポジックススレッドのmutexを使ってる

unsigned long long int nwin = 0, nlose = 0, ncan_lose = 0, nunknown = 0; // lunknownが最後のunknownの番号(-1しておく)
unsigned long long int nexist = 0;
bool flag_worker_quit = false; //仕事が終わったことを表すフラグ
int flag = 0;
unsigned long long int search_id = 0ULL;

// unsigned long long int keiro[422], length = 0/*, x*/; //-----------------------------

enum {b000 = 0, b001, b010, b011, b100, b101, b110, b111 };

ZDD& ZDD::get(int i, int j, int k, int l) noexcept {
    static ZDD inst(i, j, k+l); // private なコンストラクタを呼び出す。
    return inst;
}

// 表(2bit)を読み込むクラス
class InTable2 {
private:
    unsigned char m_buffer;
    int m_num_keep;
    fstream m_ofs;

public:
    InTable2() = delete; 
    //iter: 表を読み込むときに、今週目か, s: ファイル名, num: 配置数
    InTable2(const string &s, std::size_t num) noexcept : m_buffer(0), m_num_keep(0), m_ofs(s, fstream::in | fstream::binary) {
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
        if(num_check != num) {
            cerr << "Size Error" << endl;
            terminate();
        }
    }
    ~InTable2() noexcept {
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

//表(2bit)をメモリ上に表を持つためのクラス
class Table2 {
private:
    uint64_t *m_table;  //これで配置数分*2
    int iterations; //繰り返し回数

public:
    //iter: 今の反復回数, read_file_name: 読み込むファイルの名前, write_file_name: 書き込み先のファイルの名前
    Table2(string read_file_name) noexcept : m_table(new uint64_t [table_size64_2] ){
        cout << "read_file_name: " << read_file_name << endl;
        fstream read_file (read_file_name, fstream::in | fstream::binary);
        if(!read_file) {    //readファイルがなかった場合
            cerr << "no such file: " << read_file_name << endl;
            terminate();
        }
        cout << "aaaa" << endl;
        InTable2 in_table(read_file_name, max_table_size); //ここのInTableでエラー
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
        #ifndef USE_PURPLE
            cout << "before ncan_lose  =  " << ncan_lose << endl;
        #endif
        cout << "before nlose = " << nlose << endl;
        cout << "before nunknown  =  " << nunknown << endl;
        nwin = 0;
        ncan_lose = 0;
        nlose = 0;
        read_file.close();
    }
    ~Table2() noexcept { delete [] m_table; }

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
    Table8(int iteration, string read_file_name) noexcept : m_table(new uint64_t [table_size64_8] ){
        cout << "read_file_name: " << read_file_name << endl;
        fstream read_file (read_file_name, fstream::in | fstream::binary);
        if(!read_file) {    //readファイルがなかった場合
            cerr << "no such file: " << read_file_name << endl;
            terminate();
        }
        cout << "aaaa" << endl;
        InTable8 in_table(read_file_name, max_table_size); //ここのInTableでエラー
        cout << "bbbb" << endl;
        unsigned int count_num = 0;
        for(unsigned long long int i = 0; i < max_table_size; i++) {
            unsigned int v = in_table.read();   //in_Tableからidを一つずつ読み込んでいき、その値をvに代入
            if(v == iteration) count_num++;
            if(i < 10) cout << "v: " << v << endl;
            set(i, v);   
        }
        read_file.close();
        cout << "iteration_num:" << count_num  << endl;
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
    //通常の処理
    //argv[1] : 繰り返し回数, argv[2~5] : (i, j, k, l)
    NUM_B = atoi(argv[2]), NUM_R = atoi(argv[3]), NUM_EB = atoi(argv[4]), NUM_ER = atoi(argv[5]);
    #ifdef USE_PURPLE
        string read_file_name = "./table_purple/table" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + ".bin";
        string write_file_name = "./db_purple/db" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + ".bin";
    #else
        string read_file_name = "./table/table" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + "_p1" + ".bin";
        string write_file_name = "./db/db" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + "_p1" + ".bin";
    #endif

    ZDD::get(NUM_B, NUM_R, NUM_EB, NUM_ER).out_info();
    max_table_size = ZDD::get(NUM_B, NUM_R, NUM_EB, NUM_ER).get_num();
    table_size64_2 = (max_table_size + 31ULL) / 32ULL;
    table_size64_8 = (max_table_size + 7ULL) / 8ULL;

    int iteration = atoi(argv[1]);
    Table2 table2(read_file_name); //メモリにtableを作る
    Table8 table8(iteration, write_file_name);
    cout << "cccc" << endl;

    unsigned long long int count = 0ULL;  //前から見ていっている配置の番号
    unsigned long long int count_l = 0ULL;
    while(max_table_size > count) {    //unknownが見つかるまで
        if(table2.get(count) == NOW_SEARCH){
            if(table8.get(count) == iteration) {
                cout << "id: " << count << ": " << table2.get(count) << ": " << table8.get(count) << endl;
                ZDD::get(NUM_B, NUM_R, NUM_EB, NUM_ER).print_board(count);
                count_l++;
            }
        }
        count++;      //ここで今求めたunknownのidの値が入る
    }
    cout << count_l << endl;
    // ------------------------


    // ZDD::get(1, 1, 3).out_info();   //zddの深さとかを出力
    // POSITION posi;
    // // posi.array_red[36] = {}, posi.array_blue[36] = {}, posi.array_enemy[36] = {};

    // int len = 0;
    // unsigned char red[36] = {}, blue[36] = {}, enemy[36] = {};
    // unsigned char red_sub[38] = {}, blue_sub[38] = {}, enemy_sub[38] = {};
    // unsigned long long int id = 0ULL;
    // unsigned long long int num = ZDD::get(1, 1, 3).get_num();

    // あるidの配置を出力
    // cout << "red : △ , blue : ▲ , enemy : ▼" << endl;
    // unsigned long long int id = 656ULL;
    // cout << "―――――――――――――――――――――――――――――――――" << endl;   
    // cout << "board_number:" << id << endl; 
    // // ZDD::get(1, 1, 2).print_board(id);
    // // ZDD::get(1, 1, 2).out_info();
    // unsigned int value = database.read_database((red|player2), 0U, id);
    // cout << "value: " << value << endl;
    // ------------------------
    
    // posi = POSITION(id, 1, 1, 1, 2);
    // Action actions[max_legal_num];
    // int nchild = posi.gen_actions(actions);
    // cout << "actions size : " << nchild << endl;
    // posi.do_action(actions[2]);
    // posi.print_board();
    // Action child_actions[max_legal_num];
    // int ngrandchild = posi.gen_actions(child_actions);
    // cout << "child actions size : " << ngrandchild << endl;
    // posi.do_action(child_actions[0]);
    // posi.print_board();
    // int i = posi.compute_id();
    // cout << "number_id : " << i << endl;
    // posi.undo_action();
    // posi.undo_action();

    // //あるidの配置における合法手を出力1
    // posi = POSITION(id);
    // Action actions[max_legal_num];
    // int nchild = posi.gen_actions(actions);
    // cout << "actions size : " << nchild << endl;
    // for(int childid = 0; childid < nchild; childid++) {
    //     cout << "action number : " << childid << endl;
    //     posi.do_action(actions[childid]);
    //     posi.print_board();
    //     posi.print_capture();
    //     Action child_actions[max_legal_num];
    //     int ngrandchild = posi.gen_actions(child_actions);
    //     cout << "child actions size : " << ngrandchild << endl;
    //     // 全列挙
    //     if(ngrandchild != 0) {
    //         for(int i = 0; i < ngrandchild; i++) {
    //             posi.do_action(child_actions[i]);
    //             posi.print_board();
    //             posi.print_capture();
    //             posi.undo_action();
    //         }
    //     }
    //     // 一部列挙
    //     // if(ngrandchild != 0) {
    //     //     posi.do_action(child_actions[0]);
    //     //     int i = posi.compute_id();
    //     //     posi.print_board();
    //     //     posi.undo_action();
    //     //     posi.do_action(child_actions[ngrandchild-1]);
    //     //     int j = posi.compute_id();
    //     //     posi.print_board();
    //     //     posi.undo_action();
    //     //     cout << "first_number : " << i << ", last_number : " << j << endl;
    //     //     posi.print_board();
    //     // }
    //     posi.undo_action();
    // }
    // ------------------------

    // //あるidの配置における合法手を出力2
    // posi = POSITION(id);
    // Action actions[1024];
    // Combination combinations[70];
    // int n = posi.gen_actions(actions);
    // cout << "actions size : " << n << endl;
    // for(int i = 0; i < n; i++) {
    //     posi.do_action(actions[i]);
    //     cout << "action : " << (int)actions[i].get_before() << " → " << (int)actions[i].get_after() << endl;
    //     posi.print_board();
    //     int num_bf = posi.gen_belief_state(combinations);
    //     cout << "num belief state : " << num_bf << endl;
    //     for(int j = 0; j < num_bf; j++) {
    //         cout << "belief state : " << j << endl;
    //         cout << "do_belief_state" << endl;
    //         posi.do_belief_state(combinations[j]);
    //         posi.print_board();
    //         cout << "do_flip" << endl;
    //         posi.do_flip();
    //         posi.print_board();
    //         cout << "undo_flip" << endl;
    //         posi.undo_flip();
    //         posi.print_board();
    //         cout << "undo_belief_state" << endl;
    //         posi.undo_belief_state();
    //         posi.print_board();
    //     }
    //     posi.undo_action();
    // }
    // ------------------------


    //すべての配置を出力
    // for(unsigned long long int lp_id = 0; lp_id < num; lp_id++) {
    //     cout << "―――――――――――――――――――――――――――――――――" << endl;   
    //     cout << "board_number:" << lp_id << endl; 
    //     ZDD::get().print_board(lp_id);
    // }
    // ------------------------


    //与えたidの経路長を出力
    // id = 0ULL;
    // len = ZDD::get().compute_length(id);
    // cout << "id: " << id << ", len = " << len << endl;
    // ------------------------


    //与えたidの配列を得る
    // id = 14ULL;
    // ZDD::get().print_board(id);
    // ZDD::get().compute_array(id, white_sub, black_sub);
    
    // cout << "w: ";
    // for(int i = 0; i < 24; i++) cout << (int)white_sub[i] << " ";
    // cout << "| " << (int)white_sub[24] << " " << (int)white_sub[25] << endl;

    // cout << "b: ";
    // for(int i = 0; i < 24; i++) cout << (int)black_sub[i] << " ";
    // cout << "| " << (int)black_sub[24] << " " << (int)black_sub[25] << endl;
    // ------------------------


    // //ある配置のidを出力
    // unsigned char array_sq[40];
    // array_sq[0]  = 0U; array_sq[1]  = 0U; array_sq[2]  = 0U; array_sq[3]  = 0U; array_sq[4]  = 0U;
    // array_sq[5]  = 0U; array_sq[6]  = 0U; array_sq[7]  = 0U; array_sq[8]  = 0U; array_sq[9]  = 0U;
    // array_sq[10] = 0U; array_sq[11] = 0U; array_sq[12] = 0U; array_sq[13] = 0U; array_sq[14] = 7U;
    // array_sq[15] = 0U; array_sq[16] = 0U; array_sq[17] = 0U; array_sq[18] = 0U; array_sq[19] = 2U;
    // array_sq[20] = 0U; array_sq[21] = 0U; array_sq[22] = 0U; array_sq[23] = 0U; array_sq[24] = 0U;
    // array_sq[25] = 0U; array_sq[26] = 7U; array_sq[27] = 0U; array_sq[28] = 0U; array_sq[29] = 0U;
    // array_sq[30] = 0U; array_sq[31] = 0U; array_sq[32] = 0U; array_sq[33] = 0U; array_sq[34] = 0U;
    // array_sq[35] = 4U;

    // id = ZDD::get(1, 1, 3).compute_id(array_sq);
    // cout << "board_number: " << id << endl;
    // id = ZDD::get(1, 1, 2).compute_id(array_sq);
    // cout << "board_number: " << id << endl;
    // cout << "--------------" << endl;
    // ZDD::get(1, 1, 2).print_board(id);
    //------------------------



    //ボールをn個使っているときの各配置の数を出力  
    // int l = 0;                       　12349774257
    // for(unsigned long long int i = 0; i < 12352692569ULL; i++) {
    // for(unsigned long long int i = 12352692569ULL; i != 0ULL ; i--) {
        // FLAG = 0;
        // x = ZDD::get().compute_length(i);
        // balls[x]++;
        // length[x]++;
        // if(x == 61) {
            // l++;
            // ZDD::get().print_board(i);
            // cout << "balls[3]: " << balls[3] << endl;  
        // }
        // if(balls[9] >= 1) {
        //     for(int i = 0; i < 31; i++) {
        //     cout << "balls[" << i << "]" << balls[i] << endl;
        //     }"no changed this rupe!!"
    // }
    
    // unsigned long long int sum = 0;
    // for(int i = 0; i < 62; i++) {
        // sum += balls[i];
        // cout << "balls[" << i << "]" << balls[i] << endl;
        // cout << "balls[" << i << "]" << length[i] << en
    // }
    // cout << "sum: " << sum << endl;
    // ------------------------

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
