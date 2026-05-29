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
#include "database.hpp"
#include "posi.hpp"

#define max_legal_num 128
#define max_belief_state 128

using namespace std;

unsigned int NUM_B, NUM_R, NUM_EB, NUM_ER;

size_t max_table_size; //求める駒割の配置数
unsigned long long int table_size64_2; //全体の表2bitの方 //(12352692569ULL + 7ULL)
constexpr int nworker = 7;  //並列数
constexpr int deq_input_size = 1024;    
constexpr int deq_output_size = 256;    

condition_variable cv_boss;     //condition_variableもポジックススレッドの排他制御の一つ
condition_variable cv_worker;   
mutex mtx;  
//ポジックススレッディング
//ポジックス-インターフェースの名前
//ここではポジックススレッドのmutexを使ってる

unsigned long long int nwin = 0, nlose = 0, ncan_lose = 0, nunknown = 0, newlose = 0, lcan_lose; // lcan_loseが最後のunknownの番号(-1しておく)
unsigned long long int nexist = 0;
bool flag_worker_quit = false; //仕事が終わったことを表すフラグ
int iteration;

// unsigned long long int keiro[422], length = 0/*, x*/; //-----------------------------

enum {b000 = 0, b001, b010, b011, b100, b101, b110, b111 };

ZDD& ZDD::get(int iter, int i, int j, int k, int l) noexcept {
    int e = k + l;
    if(iter % 2 == 1) {
        static ZDD inst(i, j, e), inst_e(i, j, e-1);
        if     (i == NUM_B && j == NUM_R && e == NUM_EB+NUM_ER)   return inst;
        else if(i == NUM_B && j == NUM_R && e == NUM_EB+NUM_ER-1) return inst_e;
        else {
            cerr << "this ZDD is not accepted: " << i << ", " << j << ", " << k << ", " << l << endl;
            std::terminate();
        }
    } else {
        static ZDD inst(i, j, e), rev_inst(k, l, i+j);
        if     (i == NUM_B  && j == NUM_R  && e == NUM_EB+NUM_ER) return inst;
        else if(i == NUM_EB && j == NUM_ER && e == NUM_B+NUM_R)   return rev_inst;
        else {
            cerr << "this ZDD is not accepted: " << i << ", " << j << ", " << k << ", " << l << endl;
            std::terminate();
        }
    }
}

Database& Database::get(int iter, int i, int j, int k, int l) noexcept {
    if(iter % 2 == 1) {
        string file_name1 = "./table/table" + to_string(i) + '-' + to_string(j) + '-' + to_string(k) + '-' + to_string(l) + "_p2.bin";
        static Database table(file_name1, ZDD::get(iter, i, j, k, l).get_num());
        unsigned long long int size1, size2;
        // if (k < 2) size1 = 0;
        // else size1 = ZDD::get(iter, i, j, k-1, l).get_num();
        // if (l < 2) size2 = 0;
        // else size2 = ZDD::get(iter, i, j, k ,l-1).get_num();
        string file_name2 = "./table/table" + to_string(i) + '-' + to_string(j) + '-' + to_string(k-1) + '-' + to_string(l) + "_p2.bin";
        static Database table_k(file_name2, ZDD::get(iter, i, j, k-1, l).get_num());
        string file_name3 = "./table/table" + to_string(i) + '-' + to_string(j) + '-' + to_string(k) + '-' + to_string(l-1) + "_p2.bin";
        static Database table_l(file_name3, ZDD::get(iter, i, j, k ,l-1).get_num());

        if     (i == NUM_B && j == NUM_R && k == NUM_EB   && l == NUM_ER)   return table;
        else if(i == NUM_B && j == NUM_R && k == NUM_EB-1 && l == NUM_ER)   return table_k;
        else if(i == NUM_B && j == NUM_R && k == NUM_EB   && l == NUM_ER-1) return table_l;
        else {
            cerr << "no file: " << i << ", " << j << ", " << k << ", " << l << endl;
            std::terminate();
        }
    } else {
        string file_name1 = "./table/table" + to_string(k) + '-' + to_string(l) + '-' + to_string(i) + '-' + to_string(j) + "_p1.bin";
        static Database rev_table(file_name1, ZDD::get(iter, k, l, i, j).get_num());
        if(i == NUM_EB && j == NUM_ER && k == NUM_B && l == NUM_R) return rev_table;
        else if(i == NUM_B && j == NUM_R && k == NUM_EB && l == NUM_ER) return rev_table;
        else {
            cerr << "no file: " << i << ", " << j << ", " << k << ", " << l << endl;
            std::terminate();
        }
    }
}


// 表(2bit)を読み込むクラス
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
            cout << "header[" << i << "]: " << (int)header[i] << endl;
        }

        //headerに記憶されている配置数を取得(256進数)
        for(int i = 5; i >= 0; i--){
            num_check *= 256ULL;    
            num_check += header[i+2];
            // cout << num_check << endl;
        }
        //header[0],[1]の値を正誤判定(0回目はなしで)
        if(header[0] != 0 || header[1] == 100) {
            cerr << "Header Error" << endl;
            terminate();
        }

        iteration = header[1];
        
        //配置数の確認(header[2-7])
        if(num_check != num) {
            // cout << num_check << "a" << num << endl;
            cerr << "this Size Error" << endl;
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

//表(2bit)をメモリ上に表を持つためのクラス
class Table {
private:
    uint64_t *m_table;  //これで配置数分*2
    int iterations; //繰り返し回数

public:
    //read_file_name: 読み込むファイルの名前
    Table(string read_file_name) noexcept : m_table(new uint64_t [table_size64_2] ){
        cout << "read_file_name: " << read_file_name << endl;
        fstream read_file (read_file_name, fstream::in | fstream::binary);
        if(!read_file) {    //readファイルがなかった場合
            cout << "no file" << endl;
            for(size_t tableid = 0; tableid < table_size64_2; tableid++) m_table[tableid] = 0U; //全ての表を0にする
        } else {
            cout << "aaaa" << endl;
            InTable in_table(read_file_name, max_table_size); //ここのInTableでエラー
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
                    lcan_lose = i;
                } 
                set(i, v);   
            }
            cout << "before nwin  =  " << nwin << endl;
            cout << "before ncan_lose  =  " << ncan_lose << endl;
            cout << "before nlose = " << nlose << endl;
            cout << "before nunknown  =  " << nunknown << endl;
            cout << "last can_lose  =  " << lcan_lose << endl;
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
    unsigned int m_child[max_legal_num];  //子節点で公開可能な駒があるか否か
    unsigned int m_array_val[max_legal_num][2]; //子節点の値

public:
    Work() noexcept {}
    Work(unsigned long long int id) noexcept : m_id(id) {}
    //void set_num_child(int num_child) noexcept { m_num_child = num_child; } // delete
    void set_id(unsigned long long int id) noexcept { m_id = id;}
    void set_path_length(int path_length) noexcept { m_path_length = path_length; }
    void set_child(int num_child, unsigned int child[]) noexcept {
        m_num_child = num_child;
        assert(m_num_child > 0 && m_num_child <= max_legal_num);
        for(int childid = 0; childid < m_num_child; childid++) {
            m_child[childid] = child[childid];
        }
    }
    void set_array_val(int num_child, unsigned int child[], unsigned int array_val[][2]) noexcept {
        m_num_child = num_child;
        assert(m_num_child > 0 && m_num_child <= max_legal_num);
        for(int childid = 0; childid < m_num_child; childid++) {
            m_child[childid] = child[childid];
            for(int colorid = 0; colorid < m_child[childid]; colorid++) { // 配列の最後に青赤の分岐があるかを格納
                m_array_val[childid][colorid] = array_val[childid][colorid];
            }
        }
    }
    unsigned long long int get_id() const noexcept { return m_id; }
    int get_path_length() const noexcept { return m_path_length; }
    int get_num_child() const noexcept { return m_num_child; }
    void get_color(unsigned int child[]) const noexcept {
        assert(m_num_child <= max_legal_num);
        for(int childid = 0; childid < m_num_child; childid++) {
            child[childid] = m_child[childid];
        }
    }
    void get_array(unsigned int child[], unsigned int array_val[][2]) const noexcept {
        assert(m_num_child > 0 && m_num_child <= max_legal_num);
        for(int childid = 0; childid < m_num_child; childid++) {
            child[childid] = m_child[childid];
            assert(m_child[childid] == 1 || m_child[childid] == 2);
            for(int colorid = 0; colorid < m_child[childid]; colorid++) {
                array_val[childid][colorid] = m_array_val[childid][colorid];
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
        cout << "write" << endl;
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
    cout << "boss" << endl;
    Table table(read_file_name); //メモリにtableを作る
    // cout << "initial position   " << table.get(26439693347ULL) << endl;
    // cout << table.get(58861264410ULL) << endl;
    // cout << table.get(58861264206ULL) << endl;
    // cout << table.get(58863054486ULL) << endl;
    // cout << table.get(58884150246ULL) << endl;
    // cout << table.get(58898871716ULL) << endl;
    // cout << table.get(58885280469ULL) << endl;
    // cout << table.get(58898514174ULL) << endl;
    // cout << table.get(61502890298ULL) << endl;
    
    unsigned long long int count_changes = 0ULL; //更新があった回数(0なら異常終了, 終わり)
    unsigned long long int count_input = 0ULL;  //前から見ていっている配置の番号
    unsigned long long int count_output = 0ULL; //仕事の数、これが最後まで行ったら終了(多分)
    
    //unsigned long long int test = 0;
    int nstack_work_idle = deq_input_size + deq_output_size + nworker;  //今動いているworkの数
    Work* stack_work_idle[nstack_work_idle];    //Workの配列(workの棚のようなもの)
    for(int i = 0; i < nstack_work_idle; i++) stack_work_idle[i] = new Work;    //各棚に紙を置いとく

    //手数を記録する表を開いておく
    ofstream os(write_file_name, ios::binary | ios::in | ios::ate);
    
    while(true) {// 仕事がなくなるまで繰り返す
        unique_lock<mutex> lck(mtx); // ロック, unique_lockのインスタンスのlckが破棄されると自動的にmtxがアンロック状態になる
        cv_boss.wait(lck, [&](){     //wait(unique_lockのインスタンス, 何かしらの関数(参照,ポインタ,ラムダ式でも可)), ここだとラムダ式の参照渡し
            return (((deq_input.size() < deq_input_size) && (deq_output.size() < deq_output_size) && (count_input <= lcan_lose))
                    || (0 < deq_output.size())); });
                                        // deq_inputやdeq_outputが十分減るまで待つ(真だったら寝ないし、ロックも解放しない)
        // waitは条件を満たすまで寝る(待つ)(偽の間はずっと待ち状態)。 条件を満たすとwaitがcall_backする。
        // 条件:第二引数の関数が真偽。関数の実行はどのタイミングでもされうる(基本的にはない)。
        // waitが寝ている間は、第一引数(lck)がアンロック状態になる。       
        // ロックの解放と寝るのはアトミック(同時)
        // 起きたなら、ロックは獲得している。起きてから式の評価を1回行い、偽ならばロックを解放して再び寝る。真ならば、コールバック。
        // deq_input.size(), deq_output.size()が小さいならunknownのidを見つけて、そのworkを作る(①の処理)
        if ((deq_input.size() < deq_input_size) && (deq_output.size() < deq_output_size) && count_input <= lcan_lose) {
            // 後退解析の表を使う。
            // add only unknown id
            while(table.get(count_input) != v_can_lose) {    //can_loseが見つかるまで
                count_input++;      //ここで今求めたloseのidの値が入る
                count_output++;     //ここで今求めたloseのidの値が入る
                if(count_output % 1000000000ULL == 0ULL) cout << "count_input: " << count_input << ", nwin: " << nwin << ", ncan_lose " << ncan_lose << ", nunknown: " << nunknown << endl;

                if(count_input > (unsigned long long int)max_table_size) {
                    cout << "no can_lose id!!" << endl;
                    std::terminate();
                }
            }
            assert(table.get(count_input) == v_can_lose);
            assert(nstack_work_idle >= 1);
            Work *pw = stack_work_idle[ --nstack_work_idle ];   //割り当てるworkをstack_work_idleから持ってくる
            pw->set_id(count_input);    //workerに渡す配置番号を決定
            deq_input.push_front(pw);   //新たに仕事を追加する
            lck.unlock();
            count_input++;              //次のidは今のcount_inputの次の値なのでインクリメント
            if(count_input % 1000000000ULL == 0ULL) cout << "count_input: " << count_input << ", nwin: " << nwin << ", ncan_lose " << ncan_lose << ", nunknown: " << nunknown << endl;
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
                    bool is_lose = true;
                    if(iter % 2 == 1) {
                        int nchild = deq_tmp[workid]->get_num_child();
                        assert(nchild > 0);
                        unsigned int color[nchild], array[nchild][2];
                        deq_tmp[workid]->get_array(color, array);
                        for(int childid = 0; childid < nchild; childid++) {
                            for(int colorid = 0; colorid < color[childid]; colorid++) {
                                if(array[childid][colorid] != v_lose){
                                    is_lose = false;
                                    break;
                                }
                            }
                            if(!is_lose) break;
                        }
                    } else if(iter % 2 == 0) {
                        int nchild = deq_tmp[workid]->get_num_child();
                        assert(nchild >= 2);
                        unsigned int array[nchild];
                        deq_tmp[workid]->get_color(array);
                        for(int childid = 0; childid < nchild; childid++) {
                            if(array[childid] != v_win) {
                                is_lose = false;
                                break;
                            }
                        }
                    }
                    if (is_lose) {
                        table.set(id, v_lose);
                        count_changes++;
                        nlose++;
                        newlose++;
                        ncan_lose--;
                        os.seekp(id, ios::beg);  //手数の表のi番目を書き換える
                        char ch = (char)iter;
                        os.write(&ch, 1);
                        if(count_changes % 500000 == 0) cout << id << endl;
                    }
                }
                //紙を棚に戻す作業
                for(size_t i = 0; i < deq_tmp.size(); i++){
                    assert(nstack_work_idle < 2048);
                    stack_work_idle[ nstack_work_idle++ ] = deq_tmp[i];
                }
            }
            // 後退解析の表を使う。
            if (lcan_lose+1 <= count_output) break;  //outputがlcan_loseを超えるたら、これ以上は調べる必要がない
        }
    }   //このスコープを抜けたらlckは破棄され、mtxがアンロック状態になる

    os.close(); // 手数を記録する表を閉じる

    if(count_changes == 0) {
        cout << "no changed this rupe!!" << endl;
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

    //手数を記録する表を更新してる
    // cout << "update file: " << write_file_name << endl;
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

    //int result;

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
        if(iter % 2 == 1) {
            Action actions[max_legal_num];
            int nchild = posi.gen_actions(actions); // posiの合法手列挙
            assert(nchild > 0);
            unsigned int child_array_val[nchild][2]; // 子供の配置の値を表す配列（1個目：行動, 2個目：駒の公開）
            unsigned int color[nchild]; // 公開可能な駒があるかないかの配列（ないとき：1, あるとき：2）
            for(int childid = 0; childid < nchild; childid++) { //子供ごとの実行
                posi.do_action(actions[childid]);
                if(posi.get_last_capture_p1() == (purple|player2)) {
                    color[childid] = 2;
                    // 取った駒が青駒のとき
                    posi.do_color(blue);
                    if(posi.is_end() != 0) child_array_val[childid][0] = posi.is_end();
                    else child_array_val[childid][0] = Database::get(iter, posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).read_database(posi.compute_id());
                    assert(child_array_val[childid][0] == v_unknown || child_array_val[childid][0] == v_win || child_array_val[childid][0] == v_lose || child_array_val[childid][0] == v_can_lose);
                    posi.undo_color();
                    // 取った駒が赤駒のとき
                    posi.do_color(red);
                    if(posi.is_end() != 0) child_array_val[childid][1] = posi.is_end();
                    else child_array_val[childid][1] = Database::get(iter, posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).read_database(posi.compute_id());
                    assert(child_array_val[childid][1] == v_unknown || child_array_val[childid][1] == v_win || child_array_val[childid][1] == v_lose || child_array_val[childid][1] == v_can_lose);
                    posi.undo_color();
                } else {
                    color[childid] = 1;
                    if(posi.is_end() != 0) child_array_val[childid][0] = posi.is_end();
                    else child_array_val[childid][0] = Database::get(iter, posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).read_database(posi.compute_id());
                    assert(child_array_val[childid][0] == v_unknown || child_array_val[childid][0] == v_win || child_array_val[childid][0] == v_lose || child_array_val[childid][0] == v_can_lose);
                }
                posi.undo_action();
            }
            w->set_array_val(nchild, color, child_array_val);    //子供の数と孫の数とその配置の値の配列をセット
        } else if(iter % 2 == 0) {
            Combination combinations[max_belief_state];

            int nchild = posi.gen_belief_state(combinations); // posiのbelief-state生成
            assert(nchild >= 2);
            unsigned int array[nchild]; // 子節点の値
            for(int childid = 0; childid < nchild; childid++) {
                posi.do_belief_state(combinations[childid]);
                posi.do_flip();
                assert(posi.is_end() == 0);
                array[childid] = Database::get(iter, posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).read_database(posi.compute_id());
                assert(array[childid] == v_win || array[childid] == v_lose || array[childid] == v_can_lose || array[childid] == v_unknown);
                posi.undo_flip();
                posi.undo_belief_state();
            }
            w->set_child(nchild, array);
        }

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
        ZDD::get(NUM_B, NUM_R, NUM_EB, NUM_ER).out_info();
        Database::get(NUM_B, NUM_R, NUM_EB, NUM_ER).out_info();
        POSITION posi = POSITION();
        string s_fen(argv[1]), s_turn(argv[2]);
        posi.fen_to_array(s_fen, s_turn, NUM_B, NUM_R, NUM_EB, NUM_ER);
        cout << "print board" << endl;
        posi.print_board();
        Action actions[max_legal_num];
        int nchild = posi.gen_actions(actions);
        int ngrandchild[nchild];
        assert(nchild > 0);
        unsigned int thisresult = Database::get(NUM_B, NUM_R, NUM_EB, NUM_ER).read_database(posi.compute_id());
        if(thisresult == v_lose) {
            for(int childid = 0; childid < nchild; childid++) {
                posi.do_action(actions[childid]);
                cout << "child" << childid << ":" << endl;
                cout << "     " << posi.array_to_fen() << endl;
                posi.do_flip();
                Combination combinations[max_belief_state];
                int nbelief_state = posi.gen_belief_state(combinations);
                for(int stateid = 0; stateid < nbelief_state; stateid++) {
                    posi.do_belief_state(combinations[stateid]);
                    unsigned long long int id = posi.compute_id();
                    unsigned int value;
                    size_t table_size = Database::get(posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).get_size();
                    if(table_size < id) {
                        assert(id <= table_size + 3ULL);
                        value = id - table_size;
                        assert(value == v_win);
                    } else {
                        assert(id >= 0 && id < table_size);
                        value = Database::get(posi.get_blue(), posi.get_red(), posi.get_enemy_blue(), posi.get_enemy_red()).read_database(id);
                    }
                    if (value == v_win) {
                        cout << "grandchild" << stateid << ": " << posi.array_to_fen() << endl;
                        cout << "     result = win!!" << endl;
                    } else {
                        cout << "this arrangement is not 'lose'!!" << endl;
                        std::terminate();
                    }
                    posi.undo_belief_state();
                }
                posi.undo_flip();
                posi.undo_action();
            }
        }

        return 0;
    #endif

    //必敗を求めるループ
    //argv[1] : 繰り返し回数, argv[2~5] : (i, j, k, l)
    NUM_B = atoi(argv[2]), NUM_R = atoi(argv[3]), NUM_EB = atoi(argv[4]), NUM_ER = atoi(argv[5]);
    cout << "find lose > " << atoi(argv[1]) << ": (" << NUM_B << ", " << NUM_R << ", " << NUM_EB << ", " << NUM_ER << ")" << endl;

    ZDD::get(atoi(argv[1]), NUM_B, NUM_R, NUM_EB, NUM_ER).out_info();
    max_table_size = ZDD::get(atoi(argv[1]), NUM_B, NUM_R, NUM_EB, NUM_ER).get_num();
    table_size64_2 = (max_table_size + 15ULL) / 16ULL, lcan_lose = max_table_size-1;

    Database::get(atoi(argv[1]), NUM_B, NUM_R, NUM_EB, NUM_ER).out_info();

    string player;
    if(atoi(argv[1]) % 2 == 1) player = "p1";  // 繰り返し回数が奇数なら手番プレイヤは1
    else if(atoi(argv[1]) % 2 == 0) player = "p2"; // 繰り返し回数が偶数なら手番プレイヤは2
    string read_file_name = "./table/table" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + '_' + player + ".bin";
    string write_file_name = "./db/db" + to_string(NUM_B) + '-' + to_string(NUM_R) + '-' + to_string(NUM_EB) + '-' + to_string(NUM_ER) + '_' + player + ".bin";
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
    cout << "after nwin  =  " << nwin << endl;
    cout << "after ncan_lose  =  " << ncan_lose << endl;
    cout << "after n_lose  =  " << nlose << endl;
    cout << "after nunknown  =  " << nunknown << endl;
    cout << "new nlose  =  " << newlose << endl;
    // ------------------------

    // NUM_B = atoi(argv[1]), NUM_R = atoi(argv[2]), NUM_EB = atoi(argv[3]), NUM_ER = atoi(argv[4]);
    // ZDD::get(NUM_B, NUM_R, NUM_EB, NUM_ER).out_info();   //zddの深さとかを出力
    // POSITION posi;
    // // posi.array_red[36] = {}, posi.array_blue[36] = {}, posi.array_enemy[36] = {};

    // int len = 0;
    // unsigned char red[36] = {}, blue[36] = {}, enemy[36] = {};
    // unsigned char red_sub[38] = {}, blue_sub[38] = {}, enemy_sub[38] = {};
    // unsigned long long int id = 0ULL;
    // unsigned long long int num = ZDD::get(NUM_B, NUM_R, NUM_EB, NUM_ER).get_num();

    // //あるidの配置を出力
    // cout << "red : △ , blue : ▲ , enemy : ▼" << endl;
    // id = 106288925ULL;
    // cout << "―――――――――――――――――――――――――――――――――" << endl;   
    // cout << "board_number:" << id << endl; 
    // ZDD::get(NUM_B, NUM_R, NUM_EB, NUM_ER).print_board(id);
    // ------------------------
    
    // //あるidの配置における合法手を出力1
    // posi = POSITION(id);
    // Action actions[max_legal_num];
    // int nchild = posi.gen_actions(actions);
    // cout << "actions size : " << nchild << endl;
    // for(int childid = 0; childid < nchild; childid++) {
    //     cout << "action number : " << childid << endl;
    //     posi.do_action(actions[childid]);
    //     posi.print_board();
    //     Action child_actions[max_legal_num];
    //     int ngrandchild = posi.gen_actions(child_actions);
    //     cout << "child actions size : " << ngrandchild << endl;
    //     posi.do_action(child_actions[0]);
    //     posi.print_board();
    //     posi.undo_action();
    //     posi.do_action(child_actions[ngrandchild-1]);
    //     posi.print_board();
    //     posi.undo_action();
    //     posi.print_board();
    //     posi.undo_action();
    // }
    // ------------------------

    // // あるidの配置における合法手を出力2
    // posi = POSITION(NUM_B, NUM_R, NUM_EB, NUM_ER, id);
    // Action actions[max_legal_num];
    // Combination combinations[max_belief_state];
    // int nchild = posi.gen_actions(actions);
    // cout << "actions size : " << nchild << endl;
    // for(int i = 0; i < nchild; i++) {
    //     posi.do_action(actions[i]);
    //     unsigned long long int ac_id = posi.compute_id();
    //     cout << "action : " << i << " : " << (int)actions[i].get_before() << " → " << (int)actions[i].get_after() << " : " << ac_id << endl;
    //     posi.print_board();
    //     cout << "do_flip" << endl;
    //     posi.do_flip();
    //     posi.print_board();
    //     int nbelief_state = posi.gen_belief_state(combinations);
    //     cout << "num belief state : " << nbelief_state << endl;
    //     for(int j = 0; j < nbelief_state; j++) {
    //         cout << "belief state : " << j << endl;
    //         cout << "do_belief_state" << endl;
    //         posi.do_belief_state(combinations[j]);
    //         unsigned long long int idid = posi.compute_id();
    //         cout << "board number : " << idid << endl;
    //         posi.print_board();
    //         cout << "undo_belief_state" << endl;
    //         posi.undo_belief_state();
    //         posi.print_board();
    //     }
    //     posi.undo_flip();
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


    //ある配置のidを出力
    // white[0]  = 0; white[1]  = 0; white[2]  = 0; white[3]  = 0;
    // white[4]  = 0; white[5]  = 0; white[6]  = 0; white[7]  = 0;
    // white[8]  = 0; white[9]  = 0; white[10] = 0; white[11] = 0;
    // white[12] = 0; white[13] = 0; white[14] = 0; white[15] = 1;
    // white[16] = 0; white[17] = 0; white[18] = 1; white[19] = 0;
    // white[20] = 0; white[21] = 0; white[22] = 0; white[23] = 0;
    
    // black[0] = 0; black[1] = 0; black[2] = 0;  black[3] = 0;
    // black[4] = 0; black[5] = 0; black[6] = 0;  black[7] = 0;
    // black[8] = 0; black[9] = 0; black[10] = 0; black[11] = 0;
    // black[12] = 1; black[13] = 0; black[14] = 0; black[15] = 0;
    // black[16] = 0; black[17] = 0; black[18] = 0; black[19] = 0;
    // black[20] = 0; black[21] = 0; black[22] = 0; black[23] = 0; 

    // cout << "w: ";
    // for(int i = 0; i < 24; i++) cout << (int)white[i] << " ";
    // cout << endl;

    // cout << "b: ";
    // for(int i = 0; i < 24; i++) cout << (int)black[i] << " ";
    // cout << endl;

    // id = ZDD::get().compute_id(white, black);
    // cout << "board_number: " << id << endl;
    // cout << "--------------" << endl;
    // ZDD::get().print_board(id);
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
        //     }
        // } 
        // if(balls[14] >= 140513568) {
        //     cout << l << endl;
        //     break;
        // }
    //     if(i % 1000000000 == 0) cout << i << endl;
    // }
    
    // unsigned long long int sum = 0;
    // for(int i = 0; i < 62; i++) {
        // sum += balls[i];
        // cout << "balls[" << i << "]" << balls[i] << endl;
        // cout << "balls[" << i << "]" << length[i] << endl;
    // }
    // cout << "sum: " << sum << endl;
    // ------------------------

    return 0;
}

// g++ -O2 -o zdd.exe zdd_bg.cpp -std=c++11
// ./zdd.exe > res.txt &
