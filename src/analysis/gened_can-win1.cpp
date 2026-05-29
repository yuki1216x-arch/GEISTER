// このプログラムではv_winを勝有とする。
// また、v_unknownを勝無として、ここからv_win（勝有）を排除することで残った配置を勝無とする。
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
#include "database1.hpp"
#include "posi1.hpp"

#define max_legal_num 128
#define max_belief_state 128

using namespace std;

unsigned int NUM_B, NUM_R, NUM_EB, NUM_ER;

size_t max_table_size; //求める駒割の配置数
unsigned long long int table_size64_2; //全体の表2bitの方 //(12352692569ULL + 7ULL)
constexpr int nworker = 3;  //並列数
constexpr int deq_input_size = 1024;    
constexpr int deq_output_size = 256;

condition_variable cv_boss;     //condition_variableもポジックススレッドの排他制御の一つ
condition_variable cv_worker;   
mutex mtx;  
//ポジックススレッディング
//ポジックス-インターフェースの名前
//ここではポジックススレッドのmutexを使ってる

unsigned long long int ncan_win = 0, nlose = 0, nunknown = 0, lunknown; // lunknownが最後のunknownの番号(-1しておく)
unsigned long long int nexist = 0;
bool flag_worker_quit = false; //仕事が終わったことを表すフラグ
int flag = 0;
unsigned long long int search_id = 0ULL;

// unsigned long long int keiro[422], length = 0/*, x*/; //-----------------------------

enum {b000 = 0, b001, b010, b011, b100, b101, b110, b111 };

ZDD& ZDD::get(int iter, int i, int j, int k, int l) noexcept {
    int e = k+l;
    if(iter % 2 == 1) {
        static ZDD inst(i, j, k+l), inst_e(i, j, k+l-1), inst_flip(k, l, i+j), inst_kflip(k-1, l, i+j), inst_lflip(k, l-1, i+j); // private なコンストラクタを呼び出す。
        if     (i == NUM_B && j == NUM_R && k == NUM_EB && l == NUM_ER)   return inst;
        else if(i == NUM_B && j == NUM_R && (k == NUM_EB-1 || l == NUM_ER-1)) return inst_e;
        else if(i == NUM_EB && j == NUM_ER && k == NUM_B && l == NUM_R) return inst_flip;
        else if(i == NUM_EB-1 && j == NUM_ER && k == NUM_B && l == NUM_R) return inst_kflip;
        else if(i == NUM_EB && j == NUM_ER-1 && k == NUM_B && l == NUM_R) return inst_lflip;
        else {
            cerr << "this ZDD is not accepted: " << i << ", " << j << ", " << k << ", " << l << ", player2" << endl;
            std::terminate();
        }
    } else {
        static ZDD inst(i, j, k+l), inst_i(i-1, j, k+l), inst_j(i, j-1, k+l), inst_flip(k, l, i+j), inst_ijflip(k, l, i+j-1);
        if     (i == NUM_B   && j == NUM_R   && k == NUM_EB && l == NUM_ER) return inst;
        else if(i == NUM_B-1 && j == NUM_R   && k == NUM_EB && l == NUM_ER) return inst_i;
        else if(i == NUM_B   && j == NUM_R-1 && k == NUM_EB && l == NUM_ER) return inst_j;
        else if(i == NUM_EB && j == NUM_ER && k == NUM_B && l == NUM_R) return inst_flip;
        else if(i == NUM_EB && j == NUM_ER && (k == NUM_B-1 || l == NUM_R-1)) return inst_ijflip;
        else {
            cerr << "this ZDD is not accepted: " << i << ", " << j << ", " << k << ", " << l << ", player1" << endl;
            std::terminate();
        }
    }
}

// Database::Database(int iter, int i, int j, int k, int l) noexcept: num_b(i), num_r(j), num_eb(k), num_er(l) {
//     if(iter % 2 == 1) {
//         string file_name = "./table/table_cw" + to_string(i) + '-' + to_string(j) + '-' + to_string(k) + '-' + to_string(l) + "_p2.bin";
//         dtable = new DTable(file_name, ZDD::get(iter, i, j, k, l).get_num());
//         string file_name_flip = "./table/table" + to_string(k) + '-' + to_string(l) + '-' + to_string(i) + '-' + to_string(j) + "_p1.bin";
//         dtable_flip = new DTable(file_name_flip, ZDD::get(iter, k, l, i, j).get_num());
//         if(k < 2) dtable_cap_b = new DTable(), dtable_flip_cap_b = new DTable();
//         else {
//             string file_name_cap_b = "./table/table_cw" + to_string(i) + '-' + to_string(j) + '-' + to_string(k-1) + '-' + to_string(l) + "_p2.bin";
//             string file_name_flip_cap_b = "./table/table" + to_string(k-1) + '-' + to_string(l) + '-' + to_string(i) + '-' + to_string(j) + "_p1.bin";
//             dtable_cap_b = new DTable(file_name_cap_b, ZDD::get(iter, i, j, k-1, l).get_num());
//             dtable_flip_cap_b = new DTable(file_name_flip_cap_b, ZDD::get(iter, k-1, l, i, j).get_num());
//         }
//         if(l < 2) dtable_cap_r = new DTable(), dtable_flip_cap_r = new DTable();
//         else { 
//             string file_name_cap_r = "./table/table_cw" + to_string(i) + '-' + to_string(j) + '-' + to_string(k) + '-' + to_string(l-1) + "_p2.bin";
//             string file_name_flip_cap_r = "./table/table" + to_string(k) + '-' + to_string(l-1) + '-' + to_string(i) + '-' + to_string(j) + "_p1.bin";
//             dtable_cap_r = new DTable(file_name_cap_r, ZDD::get(iter, i, j, k, l-1).get_num());
//             dtable_flip_cap_r = new DTable(file_name_flip_cap_r, ZDD::get(iter, k, l-1, i, j).get_num());
//         }
//     } else {
//         string file_name = "./table/table_cw" + to_string(i) + '-' + to_string(j) + '-' + to_string(k) + '-' + to_string(l) + "_p1.bin";
//         dtable = new DTable(file_name, ZDD::get(iter, i, j, k, l).get_num());
//         string file_name_flip = "./table/table" + to_string(k) + '-' + to_string(l) + '-' + to_string(i) + '-' + to_string(j) + "_p2.bin";
//         dtable_flip = new DTable(file_name_flip, ZDD::get(iter, k, l, i, j).get_num());
//         if(i < 2) dtable_cap_b = new DTable(), dtable_flip_cap_b = new DTable();
//         else {
//             string file_name_cap_b = "./table/table_cw" + to_string(i-1) + '-' + to_string(j) + '-' + to_string(k) + '-' + to_string(l) + "_p1.bin";
//             string file_name_flip_cap_b = "./table/table" + to_string(k) + '-' + to_string(l) + '-' + to_string(i-1) + '-' + to_string(j) + "_p2.bin";
//             dtable_cap_b = new DTable(file_name_cap_b, ZDD::get(iter, i-1, j, k, l).get_num());
//             dtable_flip_cap_b = new DTable(file_name_flip_cap_b, ZDD::get(iter, k, l, i-1, j).get_num());
//         }
//         if(j < 2) dtable_cap_r = new DTable(), dtable_flip_cap_r = new DTable();
//         else {
//             string file_name_cap_r = "./table/table_cw" + to_string(i) + '-' + to_string(j-1) + '-' + to_string(k) + '-' + to_string(l) + "_p1.bin";
//             string file_name_flip_cap_r = "./table/table" + to_string(k) + '-' + to_string(l) + '-' + to_string(i) + '-' + to_string(j-1) + "_p2.bin";
//             dtable_cap_r = new DTable(file_name_cap_r, ZDD::get(iter, i, j-1, k, l).get_num());
//             dtable_flip_cap_r = new DTable(file_name_flip_cap_r, ZDD::get(iter, k, l, i, j-1).get_num());
//         }
//     }
// }

// table_i(i-1, j, k, l, turn), table_i_k(i-1, j, k-1, l, turn), table_i_l(i-1, j, k, l-1, turn),
                        // table_j(i, j-1, k, l, turn), table_j_k(i, j-1, k-1, l, turn), table_j_l(i, j-1, k, l-1, turn)

// 表(4bit)を読み込むクラス
class InTable {
private:
    unsigned char m_buffer;
    int m_num_keep;
    fstream m_ofs;

public:
    InTable() = delete; 
    //iter: 表を読み込むときに、今週目か, s: ファイル名, num: 配置数
    InTable(int iter, const string &s, std::size_t num) noexcept : m_buffer(0), m_num_keep(0), m_ofs(s, fstream::in | fstream::binary) {
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
        //header[0],[1]の値を正誤判定(0回目はなしで)
        if(iter != 1 && iter != 2) {
            if(header[0] != 0 || header[1] != iter-2) { 
                cerr << "Header Error" << endl;
                std::terminate();
            }
        } else {
            if(header[0] != 0 || header[1] != 0) {
                cerr << "Header Error" << endl;
                std::terminate();
            }
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
    Table(int iter, string read_file_name) noexcept : m_table(new uint64_t [table_size64_2] ){
        std::cout << "read_file_name: " << read_file_name << endl;
        fstream read_file (read_file_name, fstream::in | fstream::binary);
        if(!read_file) {    //readファイルがなかった場合
            std::cout << "no file" << endl;
            for(size_t tableid = 0; tableid < table_size64_2; tableid++) {
                m_table[tableid] = 0U; //全ての表を0にする
                nunknown = max_table_size;
            }
        }else {
            {   
                std::cout << "aaaa" << endl;
                InTable in_table(iter, read_file_name, max_table_size); //ここのInTableでエラー
                std::cout << "bbbb" << endl;
                for(unsigned long long int i = 0; i < max_table_size; i++) {
                    unsigned int v = in_table.read();   //in_Tableからidを一つずつ読み込んでいき、その値をvに代入
                    assert(v == v_win || v == v_unknown);
                    if(v == v_win) {
                        ncan_win++;
                    } else {
                        nunknown++;
                        lunknown = i;
                    }
                    set(i, v);   
                }
                std::cout << "before ncan_win  =  " << ncan_win << endl;
                // std::cout << "before nlose =" << nlose << endl;
                std::cout << "before nunknown  =  " << nunknown << endl;
                std::cout << "last unknown  =  " << lunknown << endl;
                ncan_win = 0, nlose = 0;
            }
        }
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

//仕事を表すクラス
class Work {
private:
    unsigned long long int m_id;    //割り振ったid
    int m_path_length; // use this!
    int m_num_child;   //m_idの子節点の数
    unsigned int m_state;  //belief state の数
    unsigned int m_action[max_belief_state];  //子節点で公開可能な駒があるか否か, 繰り返し奇数回のときは各belief stateで子節点がいくつあるか
    bool m_goal[max_belief_state][max_legal_num]; //ゴールしている青駒があるか
    uint8_t m_captured_piece[max_belief_state][max_legal_num]; //前のターンで取られた駒の色は
    unsigned long long int m_array[max_belief_state][max_legal_num][2]; //各値を保持する

public:
    Work() noexcept {}
    Work(unsigned long long int id) noexcept : m_id(id) {}
    //void set_num_child(int num_child) noexcept { m_num_child = num_child; } // delete
    void set_id(unsigned long long int id) noexcept { m_id = id;}
    void set_path_length(int path_length) noexcept { m_path_length = path_length; }
    void set_array(unsigned int state, unsigned int action[], bool goal[][max_legal_num], uint8_t captured_piece[][max_legal_num], unsigned long long int array[][max_legal_num][2]) noexcept {
        m_state = state;
        assert(m_state >= 2 && m_state <= max_belief_state);
        for(int stateid = 0; stateid < m_state; stateid++) {
            m_action[stateid] = action[stateid];
            assert(m_action[stateid] > 0 && m_action[stateid] <= max_legal_num);
            for(int actionid = 0; actionid < m_action[stateid]; actionid++) { // 配列の最後に青赤の分岐があるかを格納
                m_array[stateid][actionid][0] = array[stateid][actionid][0];
                m_array[stateid][actionid][1] = array[stateid][actionid][1];
                m_captured_piece[stateid][actionid] = captured_piece[stateid][actionid];
                m_goal[stateid][actionid] = goal[stateid][actionid];
            }
        }
    }
    unsigned long long int get_id() const noexcept { return m_id; }
    int get_path_length() const noexcept { return m_path_length; }
    int get_num_child() const noexcept { return m_num_child; }
    unsigned int get_num_state() const noexcept { return m_state;}
    void get_color(unsigned int action[]) const noexcept {
        // assert(m_num_child <= max_legal_num);
        for(int stateid = 0; stateid < m_state; stateid++) {
            action[stateid] = m_action[stateid];
        }
    }
    void get_array(unsigned int action[], bool goal[][max_legal_num], unsigned char captured_piece[][max_legal_num],  unsigned long long int array[][max_legal_num][2]) const noexcept {
        assert(m_state >= 2 && m_state <= max_belief_state);
        for(int stateid = 0; stateid < m_state; stateid++) {
            action[stateid] = m_action[stateid];
            assert(action[stateid] > 0 && action[stateid] <= max_legal_num);
            for(int actionid = 0; actionid < action[stateid]; actionid++) {
                array[stateid][actionid][0] = m_array[stateid][actionid][0];
                array[stateid][actionid][1] = m_array[stateid][actionid][1];
                captured_piece[stateid][actionid] = m_captured_piece[stateid][actionid];
                goal[stateid][actionid] = m_goal[stateid][actionid];
            }
        }
    }
};

//1繰り返しの最後に表(2bit)を全部書き出すクラス
class OutTable2 {
private:
    unsigned char m_buffer;
    int m_num_keep;
    fstream m_ofs;

public:
    OutTable2() = delete;
    OutTable2(int iter, const string &s, std::size_t num) noexcept : m_buffer(0), m_num_keep(0), m_ofs(s, fstream::out | fstream::binary | fstream::trunc) {
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
    ~OutTable2() noexcept {
        // output keeping data in m_buffer;
        // assert(m_num_keep == 0);
        if (! m_ofs) {
            std::cerr << "Write Error" << std::endl;
            std::terminate();
        }
        m_ofs.close();
    }
    //表(2bit)の各番地に書き込んでいく(bitesがw,l,unk)
    void write(unsigned int bites) noexcept {    
        // put bites to m_buffer
        m_buffer |= static_cast<unsigned char>(bites << (m_num_keep * 2));
        if (++m_num_keep < 4) return;
        m_ofs.write((char*)&m_buffer, 1U);
        m_num_keep = 0;
        m_buffer = 0U;
    } 

    void flush() noexcept {
        while(m_num_keep != 0) {
            write(0);
        }
    }
};

//紙()
deque<Work *> deq_input;    //ボスが仕事を格納していき、workerがここから取り出して合法手を求める
deque<Work *> deq_output;   //workerが求めたidを格納して、ボスが取り出して表に書き込んでいく

//ボスの方
static void boss(int iter, string read_file_name, string write_file_name) noexcept {
    std::cout << "boss" << endl;
    //cout << write_file_name + to_string(iter) + ".bin" << endl;
    Table table(iter, read_file_name); //メモリにtableを作る

    Database database(iter, NUM_B, NUM_R, NUM_EB, NUM_ER);

    unsigned long long int count_changes = 0ULL; //更新があった回数(0なら異常終了, 終わり)
    unsigned long long int count_newwins = 0ULL; //新しい勝有の数
    unsigned long long int count_newloses = 0ULL; //新しい必敗の数
    unsigned long long int count_input = 0ULL;  //前から見ていっている配置の番号
    unsigned long long int count_output = 0ULL; //仕事の数、これが最後まで行ったら終了(多分)
    //unsigned long long int test = 0;
    int nstack_work_idle = deq_input_size + deq_output_size + nworker;  //今動いているworkの数
    Work* stack_work_idle[nstack_work_idle];    //Workの配列(workの棚のようなもの)
    for(int i = 0; i < nstack_work_idle; i++) stack_work_idle[i] = new Work;    //各棚に紙を置いとく

    // cout << "zzzz" << endl;

    //手数を記録する表を開いておく
    ofstream os(write_file_name, ios::binary | ios::in | ios::ate);

    while(true) {// 仕事がなくなるまで繰り返す
        unique_lock<mutex> lck(mtx); // ロック, unique_lockのインスタンスのlckが破棄されると自動的にmtxがアンロック状態になる
        cv_boss.wait(lck, [&](){     //wait(unique_lockのインスタンス, 何かしらの関数(参照,ポインタ,ラムダ式でも可)), ここだとラムダ式の参照渡し
            return (((deq_input.size() < deq_input_size) && (deq_output.size() < deq_output_size) && (count_input <= lunknown))
                    || (0 < deq_output.size())); });
                                        // deq_inputやdeq_outputが十分減るまで待つ(真だったら寝ないし、ロックも解放しない)
        // waitは条件を満たすまで寝る(待つ)(偽の間はずっと待ち状態)。 条件を満たすとwaitがcall_backする。
        // 条件:第二引数の関数が真偽。関数の実行はどのタイミングでもされうる(基本的にはない)。
        // waitが寝ている間は、第一引数(lck)がアンロック状態になる。       
        // ロックの解放と寝るのはアトミック(同時)
        // 起きたなら、ロックは獲得している。起きてから式の評価を1回行い、偽ならばロックを解放して再び寝る。真ならば、コールバック。
        // deq_input.size(), deq_output.size()が小さいならunknownのidを見つけて、そのworkを作る(①の処理)
        if ((deq_input.size() < deq_input_size) && (deq_output.size() < deq_output_size) && count_input <= lunknown) {
            // 後退解析の表を使う。
            // add only unknown id
            while(table.get(count_input) != v_unknown) {    //unknownが見つかるまで
                count_input++;      //ここで今求めたunknownのidの値が入る
                count_output++;     //ここで今求めたunknownのidの値が入る
                #ifdef USE_PURPLE
                    if(count_input % 1000000000ULL == 0ULL) std::cout << "count_input: " << count_input << ", ncan_win: " << ncan_win << ", nunknown: " << nunknown << endl;
                #else
                    if(count_input % 1000000000ULL == 0ULL) std::cout << "count_input: " << count_input << ", ncan_win: " << ncan_win << ", nunknown: " << nunknown << endl;
                #endif
                if(count_input > max_table_size-1) {
                    std::cout << "no win in this rupe!!" << endl;
                    std::cout << "no changed this rupe!!" << endl;
                    std::cout << "no unknown id!!" << endl;
                    std::terminate();
                }
            }
            assert(table.get(count_input) == v_unknown);
            assert(nstack_work_idle >= 1);
            Work *pw = stack_work_idle[ --nstack_work_idle ];   //割り当てるworkをstack_work_idleから持ってくる
            pw->set_id(count_input);    //workerに渡す配置番号を決定
            deq_input.push_front(pw);   //新たに仕事を追加する
            lck.unlock();
            count_input++;              //次のidは今のcount_inputの次の値なのでインクリメント
            #ifdef USE_PURPLE
                    if(count_input % 1000000000ULL == 0ULL) std::cout << "count_input: " << count_input << ", ncan_win: " << ncan_win << ", nunknown: " << nunknown << endl;
                #else
                    if(count_input % 1000000000ULL == 0ULL) std::cout << "count_input: " << count_input << ", ncan_win: " << ncan_win << ", nunknown: " << nunknown << endl;
                #endif
            cv_worker.notify_one(); //他スレッドを起こす
        } else {    //仕事がたまって来た場合(③の処理)
            if (0 < deq_output.size()) {
                assert(0 < deq_output.size());
                deque<Work *> deq_tmp; // use swap
                swap(deq_tmp, deq_output);  
                // empty deq_output
                deq_output.clear();
                lck.unlock();   // 早くunlockするために他のスレッドが触らないdeq_tmpにswapしている
                count_output += deq_tmp.size();
                for (unsigned int workid = 0; workid < deq_tmp.size(); workid++) {
                    unsigned long long int id = deq_tmp[workid]->get_id();  //workに割り当てられている配置番号を見る
                    unsigned int nstate = deq_tmp[workid]->get_num_state();          //そのidの子供の数(合法手の数)を得る
                    assert(nstate >= 2); // belief stateは2個以上あるはず
                    unsigned int naction[nstate];
                    bool goal[nstate][max_legal_num];
                    unsigned char captured_piece[nstate][max_legal_num];
                    unsigned long long int array[nstate][max_legal_num][2];
                    deq_tmp[workid]->get_array(naction, goal, captured_piece, array);
                    #ifdef USE_PURPLE
                        bool is_win = false;
                        bool is_unknown = false;
                        bool win_flag, unknown_flag;
                        for(int childid = 0; childid < nchild; childid++) {
                            win_flag = true;
                            unknown_flag = true;
                            assert(ngrandchild[childid] > 0);
                            for(int grandchildid = 0; grandchildid < ngrandchild[childid]; grandchildid++) {
                                if(array[childid][grandchildid] != v_win) {
                                    win_flag = false;
                                    if(array[childid][grandchildid] != v_unknown) unknown_flag = false;
                                }
                            }
                            if(win_flag) {
                                is_win = true;
                                break;
                            } else if(unknown_flag) {
                                is_unknown = true;
                            }
                        }
                        if (is_win) {   //勝ちをセット
                            table.set(id, v_win);
                            count_changes++;
                            nwin++;
                            nunknown--;
                            if(count_changes % 5000000 == 0) cout << id << endl;
                        } else if (is_unknown) {    //unknownのままなら何もしない
                            //nunknown++;
                        } else {        //負けをセット
                            table.set(id, v_lose);
                            count_changes++;
                            nlose++;
                            nunknown--;
                            if(count_changes % 5000000 == 0) cout << id << endl;
                        }
                    #else
                        bool is_can_win;
                        //以下でidの結果を決めてる(3パターン)
                        // unsigned long long int array[nchild];
                        // deq_tmp[workid]->get_array_id(nchild, array);
                        // cout << "workid : " << id << endl;
                        if(iter % 2 == 1) {
                            for(int stateid = 0; stateid < nstate; stateid++) {
                                assert(naction[stateid] > 0 && naction[stateid] <= max_legal_num);
                                for(int actionid = 0; actionid < naction[stateid]; actionid++) {
                                    is_can_win = true;
                                    // 終端節点に対する計算
                                    if(goal[stateid][actionid]) { // player1が必ずゴールできるから探索終了でOK
                                        break;
                                    }
                                    if(NUM_EB == 1 && captured_piece[stateid][actionid] == blue) { // これもplayer1の勝利で終了
                                        break;
                                    } else if(NUM_ER == 1 && captured_piece[stateid][actionid] == red) { // こっちの場合はplayer1の敗北として次の行動へ
                                        is_can_win = false;
                                        continue;
                                    }
                                    // それ以外の計算
                                    // if(database.get_value(captured_piece[stateid][actionid], array[stateid][actionid][0]) == 4U) std::cout << captured_piece[stateid][actionid] << id << endl;
                                    if(database.get_value(captured_piece[stateid][actionid], array[stateid][actionid][0]) != v_win) {
                                        is_can_win = false; // 勝有でない配置が見つかったなら次のループに入ってよし
                                        continue;
                                    }
                                    unsigned int result = database.get_value_flip(captured_piece[stateid][actionid], array[stateid][actionid][1]);
                                    // if(result == 4U) std::cout << "flip" << captured_piece[stateid][actionid] << id << endl;
                                    if(result != v_lose && result != v_can_lose) {
                                        is_can_win = false; // 勝有でない配置が見つかったなら次のループに入ってよし
                                        continue;
                                    }
                                    break; // ここまでたどり着いたということはこの配置は勝有だということ
                                }
                                if(is_can_win) break; // 勝有が決まってるならbreakでええ！
                            }
                        } else if(iter % 2 == 0) {
                            for(int stateid = 0; stateid < nstate; stateid++) {
                                assert(naction[stateid] > 0 && naction[stateid] <= max_legal_num);
                                is_can_win = true;
                                for(int actionid = 0; actionid < naction[stateid]; actionid++) {
                                    // 終端節点に対する計算
                                    if(goal[stateid][actionid]) { // player2がゴールしてしまうのでこの配色は勝有ではない
                                        is_can_win = false;
                                        break;
                                    }
                                    if(NUM_B == 1 && captured_piece[stateid][actionid] == blue) { // player2に最後の青駒を取られるなら勝てない
                                        is_can_win = false;
                                        break;
                                    } else if(NUM_R == 1 && captured_piece[stateid][actionid] == red) continue; // 勝有と断定はできないので次の行動へ

                                    // それ以外の計算
                                    if(database.get_value(captured_piece[stateid][actionid], array[stateid][actionid][0]) != v_win) {
                                        is_can_win = false;
                                        break;
                                    }
                                    unsigned int result = database.get_value_flip(captured_piece[stateid][actionid], array[stateid][actionid][1]);
                                    if(result != v_lose && result != v_can_lose) {
                                        is_can_win = false;
                                        break;
                                    }
                                }
                                if(is_can_win) break; //belief state のある物理状態ですべて勝ちなら勝ち有りとして探索終了
                            }
                        }
                        if (is_can_win) {   //勝ちをセット
                            table.set(id, v_win);
                            count_changes++;
                            count_newwins++;
                            ncan_win++;
                            nunknown--;
                            os.seekp(id, ios::beg);  //手数の表のi番目を書き換える
                            char ch = (char)iter;
                            os.write(&ch, 1);
                            if(count_changes % 5000000 == 0) std::cout << id << endl;
                        } else {    //unknownのままなら何もしない
                            //nunknown++;
                        }
                    #endif
                }
                //紙を棚に戻す作業
                for(size_t i = 0; i < deq_tmp.size(); i++){
                    assert(nstack_work_idle < 2048);
                    stack_work_idle[ nstack_work_idle++ ] = deq_tmp[i];
                }
            }
            // 後退解析の表を使う。
            if (lunknown+1 <= count_output) break;  //outputがlunknownを超えるたら、これ以上は調べる必要がない
        }
    }   //このスコープを抜けたらlckは破棄され、mtxがアンロック状態になる

    os.close(); // 手数を記録する表を閉じる

    // if(count_newwins == 0) {
    //     std::cout << "no win in this rupe!!" << endl;
    // }
    if(count_changes == 0) {
        std::cout << "no changed this rupe!!" << endl;
        std::terminate();
    }

    //table.save();
    //表(2bit)に書き込んで行く
    {
      OutTable2 out_table2(iter, read_file_name, max_table_size);
      for (unsigned long long int i = 0; i < max_table_size; i++) {
        out_table2.write(table.get(i));   //id(i)の値を取得してそのままコピー 
      } 
      out_table2.flush();
    }
//必敗を求めるループは1回だけ
    // cout << "iter > last" << endl;
    // thread th_boss([=]{boss(100, argv[1], argv[2]);});    //boss側作る

    // //終了処理
    // th_boss.join(); 
    //int result;

    //手数を記録する表を更新してる
    // std::cout << "update file: " << write_file_name << endl;
    // ofstream os(write_file_name, ios::binary | ios::in | ios::ate);
    // for(unsigned long long int i = 0; i < max_table_size; i++) {
    //     if(table.get(i) != Database::get(iter, NUM_B, NUM_R, NUM_EB, NUM_ER).read_database(i)) {
    //         os.seekp(i, ios::beg);  //手数の表のi番目を書き換える
    //         char ch = (char)iter+1;
    //         os.write(&ch, 1);
    //     }
    // //result = rename(write_file_name2.c_str(), write_file_name1.c_str());
    // //if (result == 0) puts("File successfully renamed");
    // //else perror("Error renaming file");
    // }

    {
        unique_lock<mutex> lck(mtx); //ロック
        flag_worker_quit = true; //仕事が終わったことを表すフラグ
    }
    cv_worker.notify_all();
    //cout << "Bye from boss" << endl;
}

static void worker(int iter) noexcept {
    Work *w;
    while (true) { //仕事を全て終えるまで繰り返す
        // cout << "start : worker! : " << endl;
        unique_lock<mutex> lck(mtx); //ロック
        //ロック解除して寝る
        cv_worker.wait(lck, [&](){ if (0 < deq_input.size()) return true;
                                    return flag_worker_quit; }); //仕事がある or (表に記入すべきものがあり、表がkeepされていない) or 仕事が全て終わっている になるまで待つ
                                    //ロック
        if (flag_worker_quit) break;//仕事が全て完了したなら終わり
        assert(0 < deq_input.size()); //仕事があるなら
        w = deq_input.back();   //deq_inputから取ってくる(コピー)
        deq_input.pop_back();   //取ったやつを消す
        lck.unlock();//ロック解除(deq_inputを同時に触らないようにするためのロック)
        
        // 整数値から、子供の整数値列挙 or ダイレクト勝ちありを求めて w に登録
        POSITION posi(iter, NUM_B, NUM_R, NUM_EB, NUM_ER, w->get_id());     //wのidのposiを作る

        Combination combinations[max_belief_state];
        int nstate = posi.gen_belief_state(combinations);
        assert(nstate >= 2);
        bool is_goal[nstate][max_legal_num]; // ゴールしている青駒があるか否か
        uint8_t captured_piece[nstate][max_legal_num]; // 前の行動でどの駒が取られたか
        unsigned long long int array[nstate][max_legal_num][2];  // 配置のインデックスをとにかく格納
        unsigned int naction[nstate]; // 各物理状態の行動数
        for(int stateid = 0; stateid < nstate; stateid++) {
            posi.do_belief_state(combinations[stateid]);
            Action actions[max_legal_num];
            naction[stateid] = posi.gen_actions(actions); // 合法手列挙
            assert(naction[stateid] > 0);
            for(int actionid = 0; actionid < naction[stateid]; actionid++) {
                posi.do_action(actions[actionid]); // 行動実行
                is_goal[stateid][actionid] = posi.is_goal();
                if(!is_goal[stateid][actionid]) {
                    captured_piece[stateid][actionid] = posi.get_capture();
                    array[stateid][actionid][0] = posi.compute_id();
                    posi.do_flip();
                    array[stateid][actionid][1] = posi.compute_id();
                    posi.undo_flip();
                }
                posi.undo_action();
            }
            posi.undo_belief_state();
        }
        w->set_array(nstate, naction, is_goal, captured_piece, array); // 必要な情報をすべて記録

        // 8/18 
        // 配置が終端かis_end、取られた駒はどのプレイヤの何色かcaptured_piece、配置のidが欲しい場合は記録するarrayを整理した
        // 問題点(すべてクリア)
        // 1.ここままではゴールによって終端した場合の計算ができない → ゴールした駒も必要に応じて記録するように調整
        // 2.flip関数実行時に取られた駒（取った駒）の入れ替えが行われていない可能性に気づいた → flip関数の調整が必要(仮修正済み)
        // 3.Work関数の書き換えがまだ終わっていない
        // 4.bossの特に計算部分に必敗の計算も加える

        // 9/9
        // このファイルについてはある程度修正は終わったと思う
        // うまく行けばgened_L.cppはいらない
        // gened.cppについては修正が必要. databaseの作成方法なども含め色々と変更したため

        // player2手番の場合だったもの
        // Combination combinations[max_belief_state];
        // int nstate = posi.gen_belief_state(combinations);
        // assert(nstate > 1);
        // unsigned int child_array_val[nstate][max_legal_num]; // 全子供の値を格納する
        // unsigned int nchild[nstate]; // 各物理状態の子供の数
        // for(int stateid = 0; stateid < nstate; stateid++) {
        //     posi.do_belief_state(combinations[stateid]);
        //     Action actions[max_legal_num];
        //     nchild[stateid] = posi.gen_actions(actions);
        //     // if(nchild[stateid] <= 0) {
        //     //     cout << nchild[stateid] << endl;
        //     //     posi.print_board();
        //     // }
        //     assert(nchild[stateid] > 0);
        //     for(int childid = 0; childid < nchild[stateid]; childid++) {
        //         posi.do_action(actions[childid]);
        //         child_array_val[stateid][childid] = posi.is_end();
        //         if(child_array_val[stateid][childid] == 0) child_array_val[stateid][childid] = Database::get(iter, posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).read_database(posi.compute_id());
        //         assert(child_array_val[stateid][childid] == v_unknown || child_array_val[stateid][childid] == v_win || child_array_val[stateid][childid] == v_lose || child_array_val[stateid][childid] == v_can_lose);
        //         posi.undo_action();
        //     }
        //     posi.undo_belief_state();
        // }
        // w->set_array_val(nstate, nchild, child_array_val);
        
        // cout << "end : worker! : " << w->get_id() << endl;
        // ZDDの経路を辿る
        // 後退解析の表は使わない
        // w->set_path_length(length);     
        lck.lock();//ロック(deq_outputに触るため)
        deq_output.push_front(w); // 1つのスレッドしか触っちゃいけない     
        lck.unlock();//ロック解除
        cv_boss.notify_one();//起きれるやつがいたら起こしてからコールバック, notify_one()はアンロック状態で実行されないといけない
                             //cv_bossで現在waitをcallしているスレッド1つに信号が行く。
    }
}

int main(int argc, char *argv[]) {
    // fen形式を使用する場合
    #ifdef USE_FEN
        cout << "fen format use..." << endl;
        string remaining_piece = argv[3];
        NUM_B = remaining_piece[0] - '0', NUM_R = remaining_piece[1] - '0', NUM_EB = remaining_piece[2] - '0', NUM_ER = remaining_piece[3] - '0';
        // ZDD::get(NUM_B, NUM_R, NUM_EB, NUM_ER).out_info();
        // Database::get(NUM_B, NUM_R, NUM_EB, NUM_ER).out_info();
        POSITION posi = POSITION();
        string s_fen(argv[1]), s_turn(argv[2]);
        posi.fen_to_array(s_fen, s_turn, NUM_B, NUM_R, NUM_EB, NUM_ER);
        cout << "print board" << endl;
        posi.print_board();
        Action actions[max_legal_num];
        int nchild = posi.gen_actions(actions);
        int ngrandchild[nchild];
        assert(nchild > 0);
        // unsigned int thisresult = Database::get(NUM_B, NUM_R, NUM_EB, NUM_ER).read_database(posi.compute_id());
        if(thisresult == v_win) {
            bool is_win;
            for(int childid = 0; childid < nchild; childid++) {
                posi.do_action(actions[childid]);
                if(posi.get_last_capture_p1() != (red|player2)) is_win = true;
                Action child_actions[max_legal_num];
                ngrandchild[childid] = posi.gen_actions(child_actions);
                // cout << "aaaa" << endl;
                if(ngrandchild[childid] == 0) {
                    // unsigned int result = posi.compute_id() - Database::get(posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).get_size();
                    assert(result == v_win || result == v_lose || result == v_can_lose);
                    if(result != v_win) is_win = false;
                } else {
                    for(int grandchildid = 0; grandchildid < ngrandchild[childid]; grandchildid++) {
                        posi.do_action(child_actions[grandchildid]);
                        int result;
                        unsigned long long int id = posi.compute_id();
                        // size_t table_size = Database::get(posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).get_size();
                        if(table_size < id) {
                            assert(id <= table_size + 3ULL);
                            result = id - table_size;
                            assert(result == v_win || result == v_lose || result == v_can_lose);
                        } else {
                            assert(id >= 0 && id < table_size);
                            // result = Database::get(posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).read_database(id);
                        }
                        if(result != v_win) is_win = false;
                        // cout << "bbbb: " << grandchildid << endl;
                        posi.undo_action();
                    }
                }
                if(posi.get_last_capture_p1() == (blue|player2)) {
                    posi.undo_action();
                    continue;
                }
                posi.undo_action();
                // cout << is_win << endl;
                if(is_win == true) {
                    cout << "find win route!!" << endl;
                    cout << "child fen format :" << endl;
                    posi.do_action(actions[childid]);
                    cout << posi.array_to_fen() << endl;
                    if(ngrandchild[childid] != 0) {
                        cout << "grandchild fen format :" << endl;
                        for(int grandchildid = 0; grandchildid < ngrandchild[childid]; grandchildid++) {
                            posi.do_action(child_actions[grandchildid]);
                            cout << grandchildid << ": " << posi.array_to_fen() << endl;
                            posi.undo_action();
                        }
                    }
                    posi.undo_action();
                    break;
                }
                assert(childid != nchild - 1);
            }
        } else if(thisresult == v_lose) {
            bool is_lose;
            for(int childid = 0; childid < nchild; childid++) {
                posi.do_action(actions[childid]);
                Combination combinations[max_belief_state];
                int nbelief_state = posi.gen_belief_state(combinations);
            }
        } else if(thisresult == v_can_lose) {
            cout << "search exist_lose route..." << endl;
            bool is_can_lose;
            cout << "child num = " << nchild << endl;
            for(int childid = 0; childid < nchild; childid++) {
                posi.do_action(actions[childid]);
                if(posi.get_last_capture_p1() == (red|player2) && is_can_lose == true) {
                    is_can_lose = false;
                    posi.undo_action();
                    continue;
                }
                is_can_lose = false; 
                cout << "child" << childid << " : " << posi.array_to_fen() << endl;
                Action child_actions[max_legal_num];
                ngrandchild[childid] = posi.gen_actions(child_actions);
                // cout << "aaaa" << endl;
                if(ngrandchild[childid] == 0) {
                    // unsigned int result = posi.compute_id() - Database::get(posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).get_size();
                    assert(result == v_win || result == v_lose || result == v_can_lose);
                    if(result == v_can_lose || result == v_lose) {
                        cout << "grandchild" << childid << " : " << "already lose..." << endl;
                        is_can_lose = true;
                        posi.undo_action();
                        continue;
                    }
                } else {
                    for(int grandchildid = 0; grandchildid < ngrandchild[childid]; grandchildid++) {
                        posi.do_action(child_actions[grandchildid]);
                        int result;
                        unsigned long long int id = posi.compute_id();
                        // size_t table_size = Database::get(posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).get_size();
                        if(table_size < id) {
                            assert(id <= table_size + 3ULL);
                            result = id - table_size;
                            assert(result == v_win || result == v_lose || result == v_can_lose);
                        } else {
                            assert(id >= 0 && id < table_size);
                            // result = Database::get(posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).read_database(id);
                        }
                        if(result == v_can_lose || result == v_lose) {
                            cout << "grandchild" << childid << " : " << posi.array_to_fen() << endl;
                            is_can_lose = true;
                            posi.undo_action();
                            break;
                        }
                        // cout << "bbbb: " << grandchildid << endl;
                        posi.undo_action();
                    }
                    if(is_can_lose == true) {
                        posi.undo_action();
                        continue;
                    }
                }
                if(posi.get_last_capture_p1() == (blue|player2)) {
                    posi.undo_action();
                    continue;
                }
                posi.undo_action();
                cout << "search failed!!" << endl;
                std::terminate();
            }
        } else if(thisresult == v_unknown) {
            cout << "search no_lose route..." << endl;
            int count = 0;
            for(int childid = 0; childid < nchild; childid++) {
                posi.do_action(actions[childid]);
                Action child_actions[max_legal_num];
                ngrandchild[childid] = posi.gen_actions(child_actions);
                // cout << "aaaa" << endl;
                if(ngrandchild[childid] == 0) {
                    posi.undo_action();
                    continue;
                } else {
                    int successid;
                    for(int grandchildid = 0; grandchildid < ngrandchild[childid]; grandchildid++) {
                        posi.do_action(child_actions[grandchildid]);
                        int result;
                        unsigned long long int id = posi.compute_id();
                        // size_t table_size = Database::get(posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).get_size();
                        if(table_size < id) {
                            assert(id <= table_size + 3ULL);
                            result = id - table_size;
                            assert(result == v_win || result == v_lose || result == v_can_lose);
                        } else {
                            assert(id >= 0 && id < table_size);
                            // result = Database::get(posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).read_database(id);
                        }
                        if(result != v_win && result != v_unknown) {
                            successid = -1;
                            posi.undo_action();
                            break;
                        }
                        if(result == v_unknown) successid = grandchildid;
                        // cout << "bbbb: " << grandchildid << endl;
                        posi.undo_action();
                    }
                    if(successid == -1) {
                        posi.undo_action();
                        continue;
                    }
                    cout << "search success!!" << endl;
                    cout << "child" << count << " : " << posi.array_to_fen() << endl;
                    posi.do_action(child_actions[successid]);
                    cout << "grandchild" << count << " : " << posi.array_to_fen() << endl;
                    count++;
                    posi.undo_action();
                }
                posi.undo_action();
            }
            if(count == 0) {
                cout << "search failed!!" << endl;
                std::terminate();
            }
        } else {
            cout << "you should try another arrangement!!" << endl;
        }
        // cout << "actions size : " << nchild << endl;
        // posi.do_action(actions[0]);
        // cout << "do action" << endl;
        // posi.print_board();
        // cout << "chile fen format!!" << endl;
        // cout << posi.array_to_fen() << endl;
        // posi.undo_action();
        return 0;
    #endif

    // NUM_B = 1, NUM_R = 1, NUM_EB = 1, NUM_ER = 1;
    // ZDD::get(1, 1, 1, 1, 1).out_info();
    // POSITION posi(1, 1, 1, 1, 1, 12984);     //wのidのposiを作る

    // posi.print_board();
    // Combination combinations[max_belief_state];
    // int nstate = posi.gen_belief_state(combinations);
    // assert(nstate >= 2);
    // unsigned int array[nstate][max_legal_num][2];  // 配置のインデックスをとにかく格納
    // unsigned int naction[nstate]; // 各物理状態の行動数
    // for(int stateid = 0; stateid < nstate; stateid++) {
    //     posi.do_belief_state(combinations[stateid]);
    //     posi.print_board();
    //     Action actions[max_legal_num];
    //     naction[stateid] = posi.gen_actions(actions); // 合法手列挙
    //     assert(naction[stateid] > 0);
    //     for(int actionid = 0; actionid < naction[stateid]; actionid++) {
    //         posi.do_action(actions[actionid]); // 行動実行
    //         posi.print_board();
    //         posi.do_flip();
    //         cout << posi.is_end() << endl;
    //         posi.print_board();
    //         posi.undo_flip();
    //         posi.undo_action();
    //         posi.print_board();
    //         std::cout << "iityoushi!!" << endl;
    //     }
    //     posi.undo_belief_state();
    //     posi.print_board();
    // }

    //通常の処理
    //よく考えたら繰り返し回数で手番プレイヤ判別できるな
    //argv[1] : 繰り返し回数, argv[2~5] : (i, j, k, l)
    NUM_B = atoi(argv[2]), NUM_R = atoi(argv[3]), NUM_EB = atoi(argv[4]), NUM_ER = atoi(argv[5]);
    string player;
    if(atoi(argv[1]) % 2 == 1) player = "p1";  // 繰り返し回数が奇数なら手番プレイヤは1
    else if(atoi(argv[1]) % 2 == 0) player = "p2"; // 繰り返し回数が偶数なら手番プレイヤは2
    std::cout << "iter > " << atoi(argv[1]) << ": (" << NUM_B << ", " << NUM_R << ", " << NUM_EB << ", " << NUM_ER << ")" << endl;
    #ifdef USE_PURPLE
        string read_file_name = "./table_purple/table_cw" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + '_' + player + ".bin";
        string write_file_name = "./db_purple/db_cw" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + '_' + player + ".bin";
    #else
        string read_file_name = "./table/table_cw" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + '_' + player + ".bin";
        string write_file_name = "./db/db_cw" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + '_' + player + ".bin";
    #endif

    ZDD::get(atoi(argv[1]), NUM_B, NUM_R, NUM_EB, NUM_ER).out_info();
    max_table_size = ZDD::get(atoi(argv[1]), NUM_B, NUM_R, NUM_EB, NUM_ER).get_num();
    table_size64_2 = (max_table_size + 31ULL) / 32ULL, lunknown = max_table_size-1;

    // 1周目のみこれをやる
    if(atoi(argv[1]) == 1) {

        #ifdef USE_PURPLE
            string read_file_name2 = "./table_purple/table" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + "_p2" + ".bin";
            string write_file_name2 = "./db_purple/db" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + "_p2" + ".bin";
        #else
            string read_file_name2 = "./table/table_cw" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + "_p2" + ".bin";
            string write_file_name2 = "./db/db_cw" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + "_p2" + ".bin";
        #endif

        // p1のデータベースを作る（勝敗データベースと手数データベース）
        OutTable2 out_table2a(0, read_file_name, max_table_size);
        for (unsigned long long int i = 0; i < max_table_size; i++) {
            out_table2a.write(v_unknown);   //id(i)の値を取得してそのままコピー 
        } 
        out_table2a.flush();

        ofstream osa(write_file_name, ios::binary | ios::out | ios::trunc);
        for(unsigned long long int i = 0; i < max_table_size; i++) {
            char ch = 0;
            osa.write(&ch, 1);
        }
        osa.flush();

        // p2のデータベースを作る（勝敗データベースと手数データベース）
        OutTable2 out_table2b(0, read_file_name2, max_table_size);
        for (unsigned long long int i = 0; i < max_table_size; i++) {
            out_table2b.write(v_unknown);   //id(i)の値を取得してそのままコピー 
        } 
        out_table2b.flush();

        ofstream osb(write_file_name2, ios::binary | ios::out | ios::trunc);
        for(unsigned long long int i = 0; i < max_table_size; i++) {
            char ch = 0;
            osb.write(&ch, 1);
        }
        osb.flush();
    }
    //------------------------------------------------------------------------------
    
    thread th_boss([=]{boss(atoi(argv[1]), read_file_name, write_file_name);});    //boss側作る

    thread th_worker[nworker];  //worker側を作る
    for(int workerid = 0; workerid < nworker; workerid++){
        th_worker[workerid] = thread([&]{worker(atoi(argv[1]));});   //ここでworker()を呼び出す
    }

    //終了処理
    th_boss.join(); 
    for(int workerid = 0; workerid < nworker; workerid++){
        th_worker[workerid].join();
    }
    std::cout << "after ncan_win  =  " << ncan_win << endl;
    // std::cout << "after nlose =  " << nlose << endl;
    std::cout << "after nunknown  =  " << nunknown << endl;
    // ------------------------


    // ZDD::get(atoi(argv[1]), 1, 1, 1, 1).out_info();   //zddの深さとかを出力
    // POSITION posi;
    // posi.array_red[36] = {}, posi.array_blue[36] = {}, posi.array_enemy[36] = {};

    // int len = 0;
    // unsigned char red[36] = {}, blue[36] = {}, enemy[36] = {};
    // unsigned char red_sub[38] = {}, blue_sub[38] = {}, enemy_sub[38] = {};
    // unsigned long long int id = 0ULL;
    // unsigned long long int num = ZDD::get(atoi(argv[1]), 1, 1, 1, 1).get_num();

    // あるidの配置を出力
    // cout << "red : △ , blue : ▲ , enemy : ▼" << endl;
    // unsigned long long int id = 656ULL;
    // cout << "―――――――――――――――――――――――――――――――――" << endl;   
    // cout << "board_number:" << id << endl; 
    // ZDD::get(1, 1, 2).print_board(id);
    // ZDD::get(1, 1, 2).out_info();
    // unsigned int value = database.read_database((red|player2), 0U, id);
    // cout << "value: " << value << endl;
    // ------------------------
    
    // posi = POSITION(atoi(argv[1]), 1, 1, 1, 2, id);
    // cout << "aaa" << endl;
    // posi.print_board();
    // Action actions[max_legal_num];
    // int nchild = posi.gen_actions(actions);
    // cout << "actions size : " << nchild << endl;
    // for(int childid = 0; childid < nchild; childid++) {
    //     posi.do_action(actions[childid]);
    //     if(posi.get_last_capture_p1() == (purple|player2)) {
    //         posi.do_color(blue);
    //         posi.print_board();
    //         posi.undo_color();
    //         posi.do_color(red);
    //         posi.print_board();
    //         posi.undo_color();
    //     } else posi.print_board();
    //     posi.undo_action();
    // }

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

// -std=c++11

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
