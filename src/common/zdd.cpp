#include <deque>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <map>
#include <stdint.h>
#include <climits>
#include <exception>
#include "zdd.hpp"

#define MAX_NB 4         // 青駒の最大数
#define MAX_NR 4         // 赤駒の最大数
#define MAX_NE 8         // 敵駒の最大数
#define MAX_POINT 36     // ポイント(マス)の数

using namespace std;

//節点nからx(0/1)枝に行ったときに、その節点が0葉かどうかの判定
//d:nの深さ
static bool IsNextLeaf0(const Node* n, int d, int x, unsigned char k, unsigned char rest_nb, unsigned char rest_nr, unsigned char rest_ne){
    assert(n);
    assert(rest_nb - n->nb >= 0);
    assert(rest_nr - n->nr >= 0);
    assert(rest_ne - n->ne >= 0);

    //同じポイントについて、2度1-枝へ進むとき(ガイスターでは必要ない)
    //if(x == 1 && n->f == 1) return true;    

    // if(d == 1) cout << "pieces: " << (int)piece << ", nw = " << (int)n->nw << ", nb = " << (int)n->nb << endl;
    //(d%k == 0)の節点は敵味方の分岐の節点
    //(d%k == 1)の節点は前の分岐で味方を選んだ場合の青赤の分岐の節点
    //(d%k == k-1)の節点はそのポイントでの最後の節点

    if(rest_nb == 0 && rest_nr == 0 && rest_ne < 2) {
        if(d%k == k-1 && x == 1) return true;
        if(n->kind_piece == 1) return true;
    } else {
        //1-枝へ進み、味方の各色の駒が4個超過使用されるか敵駒が8個超過使用されるとき
        if(d%k == 1 && n->kind_piece == 1 && x == 1 && rest_ne - n->ne - 1 < 0) return true;
        if(d%k == 2 && n->kind_piece == 2 && x == 1 && rest_nb - n->nb - 1 < 0) return true;
        if(d%k == 3 && n->kind_piece == 3 && x == 1 && rest_nr - n->nr - 1 < 0) return true;
        
        if(d%k != 0 && n->kind_piece == 0 && x == 1) return true;   //空マス側の木で1-枝を選んだとき、0-葉にする
        if(d%k != 0 && n->f == 1 && x == 1) return true;            //すでにオブジェクトが置かれたポイントで1-枝を選んだとき、0-葉にする
        if(d%k == 3 && n->kind_piece == 3 && x == 0) return true;   //空マスでないポイントに何も置かれなかったとき0-葉にする
        if(d%k == 0 && x == 0 && (MAX_POINT - d/k) - (rest_nb - n->nb) - (rest_nr - n->nr) - (rest_ne - n->ne) -1 < 0) return true;
        if(d%k == 0 && x == 1 && (MAX_POINT - d/k) - (rest_nb - n->nb) - (rest_nr - n->nr) - (rest_ne - n->ne) < 0) return true;
        //ポイントが変わったとき、残りのポイント(マス)に対し残りの駒をすべて配置できないとき、0-葉にする
    }

    return false;
}

//節点の特定の要素を出力する
// static void print_member(Node* n) {
//     cout << "nb: " << (int)n->nb;
//     cout << ", nw: " << (int)n->nw ;
//     cout << ", f: " << (int)n->f ;
//     cout << ", topf: " << (int)n->topf;
//     cout << ", put_ball: ";
//     cout << bitset<1>(n->put_ball >> 29) << " "; //1段目
//     cout << bitset<4>(n->put_ball >> 25) << " "; //2段目
//     cout << bitset<9>(n->put_ball >> 16) << " "; //3段目
//     cout << bitset<16>(n->put_ball);      //4段目
//     cout << endl;
// }

//3種類の経路長を求める関数
static void dfs(Node* n){
    //stack<Node*> S;
    //Node* n;
    //S.push(root);
    if(n->left->visited != 1) dfs(n->left);
    if(n->right->visited != 1) dfs(n->right);
    n->num = n->left->num + n->right->num;
    n->lengthmax = max(n->left->lengthmax, n->right->lengthmax) + 1;         
    n->lengthmin = min(n->left->lengthmin, n->right->lengthmin) + 1;
    n->lengthsum = n->left->lengthsum + n->right->lengthsum + n->num;
    n->visited = 1;
}

//zddを作る関数
static Node* construct_zdd(unsigned char rest_nb, unsigned char rest_nr, unsigned char rest_ne) noexcept {
    Node* root;                 //根節点
    root = new Node;            

    unsigned char k = 4;        //1つのポイントについて、分岐1：空マスか、分岐2：敵駒か、分岐3：青駒か、分岐4：赤駒か
    // cout << "k = " << (int)k << endl;
    deque<Node*> N[MAX_POINT*k];       //N[i]:深さiの節点集合

    Node* l0 = new Node;        //0葉
    Node* l1 = new Node;        //1葉
    Node* n;
    int nnode = 1;

    //初期化
    root->nb = 0;
    root->nr = 0;
    root->ne = 0;
    // root->eb = 0;
    // root->er = 0;
    root->kind_piece = 0;
    root->f = 0;
    root->num = 0;
    root->point_id = 0;
    root->ncheckers = 0;

    //0-葉
    l0->nb = 50;    //(+30)
    l0->nr = 50;    
    l0->ne = 50;
    // l0->eb = 50;
    // l0->er = 50;
    l0->kind_piece = 50;
    l0->f = 50;
    l0->point_id = 50;
    l0->ncheckers = 50;
    l0->num = 0;
    l0->visited = 1;
    l0->lengthmax = -500;
    l0->lengthmin = 500;
    l0->lengthsum = 0;
    l0->left = NULL;
    l0->right = NULL;

    //1-葉
    l1->nb = 60;
    l1->nr = 60;
    l1->ne = 60;
    // l1->eb = 60;
    // l1->er = 60;
    l1->kind_piece = 60;
    l1->f = 60;
    l1->point_id = 60;
    l1->ncheckers = 60;
    l1->num = 1;
    l1->visited = 1;
    l1->lengthmax = 0;
    l1->lengthmin = 0;
    l1->lengthsum = 0;
    l1->left = NULL;
    l1->right = NULL;

    N[0].push_back(root);   //N[0]に根節点を入れる
    for(int d = 0; d < MAX_POINT*k; d++) {
        int point = d / k;          //ポイント(マス)の番号
        //int piece = k - (d % k);    //駒の数
        map<unsigned int, unsigned int> for_share_node; //等価節点の共有のためのmap
        
        // cout << "point: " << point << endl;

        //深さdの各節点について
        for(size_t node_index = 0; node_index < N[d].size(); node_index++) {
            //1つ取り出す
            n = N[d][node_index];

            //0,1-枝へ進む
            for(int x = 0; x < 2; x++) {
                //0-葉へ到達するか
                if(IsNextLeaf0(n, d, x, k, rest_nb, rest_nr, rest_ne)) {  //x-枝の先が0-枝のとき
                    if(x == 0) n->left = l0;
                    else n->right = l0;
                    continue;
                }

                //深さ最大のとき
                if(d == MAX_POINT*k-1) {
                    if(x == 0) n->left = l1;
                    else n->right = l1;
                    continue;
                }

                //枝の先が0-枝でも、深さ最大でもないとき
                Node *c = new Node;   //節点を生成
                nnode++;

                //生成した節点のメンバの初期化
                //(d%k == 0)の節点は黒白の分岐の節点
                //(d%k == k-1)の節点はそのポイントでの最後の節点

                //neについて
                c->ne = n->ne;
                if(d % k == 1)     //敵駒かを判定する節点のとき
                    if(x == 1)                       //1-枝のとき
                        c->ne++;                     //敵駒の数を加算

                //nbについて
                c->nb = n->nb;
                if(d % k == 2)     //青駒かを判定する節点のとき
                    if(x == 1 && n->f != 1)          //1-枝で他の駒が置かれていないとき
                        c->nb++;                     //青駒の数を加算

                //nrについて
                c->nr = n->nr;
                if(d % k == 3)     //赤駒かを判定する節点のとき
                    if(x == 1 && n->f != 1)          //1-枝で他の駒が置かれていないとき
                        c->nr++;                     //赤駒の数を加算

                //kind_pieceについて
                c->kind_piece = n->kind_piece;
                if(d % k == 0) {    //空マスかを判定する節点のとき
                    if(x == 0) c->kind_piece = 0;   //0-枝なら空マス
                    else       c->kind_piece = 1;   //1-枝ならとりあえず敵駒
                }
                if(d % k == 1 && n->kind_piece == 1)    //置く駒が敵駒かを判定する節点で、紫駒が置かれているとき
                    if(x == 0)                      //0-枝のとき
                        c->kind_piece = 2;          //とりあえず青駒
                if(d % k == 2 && n->kind_piece == 2)    //置く駒が青細かを判定する節点で、青駒が置かれているとき
                    if(x == 0)                      //0-枝のとき
                        c->kind_piece = 3;          //とりあえず赤駒
                
                //fについて
                c->f = 0;   //一度0で初期化
                if(d%k != 0 && d%k != k-1 && (n->f == 1 || x == 1)) c->f = 1;   //節点n,cが置く駒を決める節点でなく、nのfが1か1-枝で進むとき、f = 1
                
                //point_idについて
                if(d%k != k-1) c->point_id = point;    //子が次のポイントでないならそのまま
                else c->point_id = point+1;            //次のポイント
                
                //ncheckersについて
                if(d%k != k-1) c->ncheckers = n->ncheckers + 1;    //子が次のポイントでないなら1加算
                else c->ncheckers = 0;                             //次のポイントなら0にする

                //num, visitedについて
                c->num = 0;
                c->visited = 0;

                //深さd+1に節点がないなら新しく登録
                if(N[d+1].size() == 0){     
                    N[d+1].push_back(c);    //次の深さの節点集合に追加
                    if(x == 0) n->left = c; //親と子をつなぐ
                    else n->right = c;
                    continue;
                }
                   
                //等価節点の共有
                while (true) {
                    bool flag = false;  //共有するかどうかのflag(trueなら共有する)
                    unsigned int key = 2048*c->kind_piece + 64*c->ne + 8*c->nb + c->nr;    //keyを求める
                    
                    auto it = for_share_node.find(key);      //同じkeyを探す
                    if(it != for_share_node.end()) flag = true; //itが末尾でないなら、同じkeyがあった->共有する
                    
                    //flagがfalseなら共有しない
                    if(!flag) {
                        if(N[d+1].size() > UINT_MAX) {
                            cerr << "too large size!" << endl;
                            terminate();
                        }
                        for_share_node.insert(make_pair(key, N[d+1].size()));    //新しい要素を挿入
                        N[d+1].push_back(c);
                        if(x == 0) n->left = c;
                        else n->right = c;
                        break;
                    }
                    
                    //flagがtrueの場合、共有する
                    if(x == 0) n->left = N[d+1][it->second];
                    else n->right = N[d+1][it->second];
                    delete c;
                    nnode--;
                    break;
                }
            }   
        }
    }

    //int sum = 0;

    /*for(int i = 0; i < MAX_POINT*k; i++){
        cout << "N[" << i << "]" << N[i].size() << endl;
        sum += N[i].size();
    }
    cout << "sum = " << sum << endl;
    */

    int end = 1;
    //冗長節点の削除
    while(end == 1){
        end = 0;
        for(int i = 0; i < MAX_POINT*k-1; i++){
            for(size_t j = 0; j < N[i].size(); j++){
                if(N[i][j]->left->f != 50 && N[i][j]->left->f != 60 && N[i][j]->f != 70){
                    if(N[i][j]->left->right->f == 50){
                        end = 1;
                        N[i][j]->left->f = 70;
                        N[i][j]->left = N[i][j]->left->left;
                    }
                }
                if(N[i][j]->right->f != 50 && N[i][j]->right->f != 60 && N[i][j]->f != 70){
                    if(N[i][j]->right->right->f == 50){
                        end = 1;
                        N[i][j]->right->f = 70;
                        N[i][j]->right = N[i][j]->right->left;
                    }
                }
            }
        }
    }
    for(int i = 0; i < MAX_POINT*k; i++){
        for(size_t j = 0; j < N[i].size(); j++){
            if(N[i][j]->f == 70){
                N[i].erase(N[i].begin() + j);
                j--;
            }
        }
    }

    dfs(root);

    // int sum = 0;

    // for(int i = 0; i < MAX_POINT*k; i++){
    //     sum += N[i].size();
    //     cout << "N[" << i << "]: " << N[i].size() << endl;
    // }
    // cout << "sumad =" << sum << endl;   //節点数

    return root;
}

ZDD::ZDD() noexcept {
    m_root = construct_zdd(REST_NB, REST_NR, REST_NE);
} // コンストラクタを private に置く。

ZDD::ZDD(int i, int j, int k) noexcept {
    REST_NB = i, REST_NR = j, REST_NE = k;
    m_root = construct_zdd(REST_NB, REST_NR, REST_NE);
}

ZDD::~ZDD() noexcept {} // デストラクタを private に置く。

// ZDD& ZDD::get(int i, int j, int k, int l) noexcept {
//     static int NUM_B = i, NUM_R = j, NUM_EB = k, NUM_ER = l;  // 必ず最初に基となるi,j,kで呼び出す
//     #ifdef SEARCH_LOSE
//         static ZDD inst(i, j, k+l), inst_k(i, j, k+l-1),
//                 rev_inst(k, l, i+j), rev_inst_i(k-1, l, i+j), rev_inst_j(k, l-1, i+j); // private なコンストラクタを呼び出す。
//         int e = k + l;
//         if     (i == NUM_B   && j == NUM_R   && e == NUM_EB+NUM_ER)   return inst;
//         else if(i == NUM_B   && j == NUM_R   && e == NUM_EB+NUM_ER-1)   return inst_k;
//         else if(i == NUM_EB   && j == NUM_ER && e == NUM_B+NUM_R)   return rev_inst;
//         else if(i == NUM_EB-1   && j == NUM_ER   && e == NUM_B+NUM_R) return rev_inst_i;
//         else if(i == NUM_EB && j == NUM_ER-1   && e == NUM_B+NUM_R) return rev_inst_j;
//         else assert(false);
//     #else
//         int e = k+l;
//         static ZDD inst(i, j, e), inst_i(i-1, j, e), inst_j(i, j-1, e),
//                 inst_k(i, j, e-1), inst_i_k(i-1, j, e-1), inst_j_k(i, j-1, e-1); // private なコンストラクタを呼び出す。
//         if     (i == NUM_B   && j == NUM_R   && e == NUM_EB+NUM_ER)   return inst;
//         else if(i == NUM_B-1 && j == NUM_R   && e == NUM_EB+NUM_ER)   return inst_i;
//         else if(i == NUM_B   && j == NUM_R-1 && e == NUM_EB+NUM_ER)   return inst_j;
//         else if(i == NUM_B   && j == NUM_R   && e == NUM_EB+NUM_ER-1) return inst_k;
//         else if(i == NUM_B-1 && j == NUM_R   && e == NUM_EB+NUM_ER-1) return inst_i_k;
//         else if(i == NUM_B   && j == NUM_R-1 && e == NUM_EB+NUM_ER-1) return inst_j_k;
//         else assert(false);
//     #endif
// }

//引数のidの経路長を得る
int ZDD::compute_length(unsigned long long int id) const noexcept {
    int length = 0;
    unsigned long long int x = id;
    Node* r = m_root;
    while(1){
        assert(r->f == 60 || r->f == 0 || r->f == 1);
        if(r->f == 60) {
            break;
        } else if(r->left->num <= x) {
            x -= r->left->num;
            length++;
            r = r->right;
        } else {
            r = r->left;
            length++;
        }
    }
    return length;
}

void ZDD::compute_array(unsigned long long int id, unsigned char array_sq[40]) const noexcept {
    unsigned long long int x = id;
    const Node *r = m_root;
    unsigned char nb = 0, nr = 0, ne = 0;

    for(int i = 0; i < 36; i++) array_sq[i] = 0U;

    while(true) {
        assert(r);
        if(r->f == 60) break;  // もし1-葉なら
        assert(r->f != 50);
        assert(r->f == 0 || r->f == 1);
        if(r->left->num <= x) {
            x -= r->left->num;
            if(r->ncheckers == 1) {
                array_sq[r->point_id] = (purple|player2);
                ne++; // 敵駒があるならその後2回はleftを選ぶことになる
            } else if(r->ncheckers == 2) {
                array_sq[r->point_id] = (blue|player1);
                nb++; // 青駒があるならその後1回はleftを選ぶことになる
            } else if(r->ncheckers == 3) {
                array_sq[r->point_id] = (red|player1);
                nr++;
            }
            r = r->right;
        } else {
            r = r->left;
        }
    }
    array_sq[36] = 0U, array_sq[37] = 0U, array_sq[38] = 0U, array_sq[39] = 0U;
    assert(nr == REST_NR && nb == REST_NB && ne == REST_NE);
}

unsigned long long int ZDD::compute_id(unsigned char array_sq[36]) const noexcept {
    const Node* r = m_root;
    unsigned long long int index = 0ULL;
    while(r->f < 50) {
        if(r->ncheckers == 0) {
            if(array_sq[r->point_id] == 0U) {
                r = r->left; //空マスなら敵駒、青駒、赤駒はすべて置かれない
            } else {
                index += r->left->num;
                r = r->right;
            }
        } else if(r->ncheckers == 1) {
            if(array_sq[r->point_id] == (purple|player2) || array_sq[r->point_id] == (blue|player2) || array_sq[r->point_id] == (red|player2)) {
                index += r->left->num;
                r = r->right; // 敵駒があるならその後2回はleftを選ぶことになる
            } else r = r->left;
        } else if(r->ncheckers == 2) {
            if(array_sq[r->point_id] == (blue|player1)) {
                index += r->left->num;
                r = r->right; // 青駒があるならその後1回はleftを選ぶことになる
            } else r = r->left;
        } else if(r->ncheckers == 3) {
            if(array_sq[r->point_id] == (red|player1)) {
                index += r->left->num;
                r = r->right;
            } else r = r->left;
        }
    }
    return index;
}

void pnum(unsigned char num, bool f) {
    if(num == 0 && !f) cout << " 0";
    else if(num < 10) cout << " " << (int)num;
    else         cout << (int)num;
}

void pbre(unsigned char b, unsigned char r, unsigned char e) {
    if(b == 0 && r == 0 && e == 0) {
        cout << "丶";
    }
    else if(b == 1) {
        cout << "▲";
    } else if(r == 1) {
        cout << "△";
    } else if(e == 1) {
        cout << "▼";
    } else {
        cout << "丶";
    }
}

void pbre(unsigned char b, unsigned char r, unsigned char eb, unsigned char er) {
    if(b == 0 && r == 0 && eb == 0 && er == 0) {
        cout << " ";
    }
    else if(b == 1) {
        cout << "▲";
    } else if(r == 1) {
        cout << "△";
    } else if(eb == 1) {
        cout << "▼";
    } else if(er == 1) {
        cout << "▽";
    } else {
        cout << "　";
    }
}

//整数値を受け取って、その整数値に対応する配置を出力する関数
void ZDD::print_board(unsigned long long int id) const noexcept {
    unsigned long long int x = id;

    unsigned char blue[36] = {};   //青の配置を記憶
    unsigned char b_count = MAX_NB - REST_NB;  //青を盤に置いた数
    //unsigned char b_bar = 0;    //黒のバーの数

    unsigned char red[36] = {};   //赤の配置を記憶
    unsigned char r_count = MAX_NR - REST_NR;  //赤を盤に置いた数
    //unsigned char w_bar = 0;    //白のバーの数

    unsigned char enemy[36] = {};   //敵の配置を記憶
    unsigned char e_count = MAX_NE - REST_NE;   //敵を盤に置いた数

    Node* r = m_root;
    while(1){
        assert(r->f == 60 || r->f == 0 || r->f == 1);
        assert(b_count <= 4 && r_count <= 4 && e_count <= 8);
        // cout << (int)b_count << (int)red_count << (int)e_count << endl;
        
        if(r->f == 60) {
            break;
        } else if(r->left->num <= x) {
            x -= r->left->num;
            if(r->ncheckers == 1) {
                enemy[r->point_id] = 1;
                e_count++;
            } else if(r->ncheckers == 2) {
                blue[r->point_id] = 1;
                b_count++;
            } else if(r->ncheckers == 3) {
                red[r->point_id] = 1;
                r_count++;
            }
            r = r->right;
        } else {
            r = r->left;
        }
    }
    //w_bar = 15-w_count;
    //b_bar = 15-b_count;

    // cout << "w_count = " << (int)w_count << ", b_count = " << (int)b_count << endl;
    // cout << "w: ";
    // for(int i = 0; i < 24; i++) cout << (int)w[i] << " ";
    // cout << endl;

    // cout << "b: ";
    // for(int i = 0; i < 24; i++) cout << (int)b[i] << " ";
    // cout << endl;

    //cout << "r_num = " << (int)CAPTURE_NR << ", b_num = " << (int)CAPTURE_NB << ", e_num = " << (int)CAPTURE_NE << endl; 
    cout << "   6 5 4 3 2 1" << endl;
    cout << "  |―――――――――――|" << endl;  
    for(int p = 6; p > 0; p--) {
        cout << p << " |";
        for(int q = 1; q <= 6; q++) {
            pbre(blue[6*p-q], red[6*p-q], enemy[6*p-q]);
            // if(q != 6) cout << " ";
        }
        cout << "|" << endl;
    }
    cout << "  |―――――――――――|" << endl;
}

void ZDD::out_info() const noexcept {
    // cout << "REST_NB: " << REST_NB << ", REST_NR: " << REST_NR << ", REST_NE: " << REST_NE << endl; 
    // cout << "root->num = " << m_root->num << endl;
    // cout << "root->lengthmax = " << m_root->lengthmax << endl;
    // cout << "root->lengthmin = " << m_root->lengthmin << endl;
    // cout << "root->lengthave = " << (double)m_root->lengthsum / (double)m_root->num << endl;
}

int ZDD::get_rest_num(unsigned int kind_piece) const noexcept {
    if(kind_piece == (blue|player1)) {
        return REST_NB;
    } else if(kind_piece == (red|player1)) {
        return REST_NR;
    } else if(kind_piece == (purple|player2)) {
        return REST_NE;
    } else {
        assert(false);
    }
}