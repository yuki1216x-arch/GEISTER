#ifndef ZDD_HPP
#define ZDD_HPP
#include <cassert>
#include <bitset>
#include <stdint.h>

constexpr unsigned int purple=6U;
constexpr unsigned int blue=2U;
constexpr unsigned int red=4U;
constexpr unsigned int player1=0U;
constexpr unsigned int player2=1U;

//zddの節点
struct Node{
    unsigned char nb;           //現節点までの青駒の数
    unsigned char nr;           //現節点までの赤駒の数
    unsigned char ne;           //現節点までの敵駒の数
    // unsigned char eb;           //現節点までの敵青駒の数
    // unsigned char er;           //現節点までの敵赤駒の数
    unsigned char kind_piece;   //今置こうとしている駒が青か赤か敵駒か(0:駒なし, 1:敵, 2:青, 3:赤)
    unsigned char f;            //そのマスにオブジェクトを置いたかどうか(0/1)(同じマスに2種類以上のオブジェクトは来ない)
    unsigned char point_id;     //現節点の盤面のポイント番号(0-35)
    unsigned char ncheckers;    //現節点のオブジェクト番号(0-3),0なら空マスかを決める節点,1なら敵駒かを決める節点,2なら青駒かを決める節点,3なら赤駒かを決める節点
    
    unsigned long long int num;         //その節点から到達できる葉が1になっている葉の数(盤面の配置の数)
    unsigned char visited;              //経路長を求めるためのdfsのため
    int lengthmax;                      //最大経路長ための値保持
    int lengthmin;                      //最小経路長ための値保持
    unsigned long long int lengthsum;   //平均経路長ための合計の経路長

    Node *left;     //0-枝
    Node *right;    //1-枝
};
// 8bit: 7  = 56bit
// 32bit: 3 = 96bit
// 64bit: 2 = 128bit

//ZDDのクラス
class ZDD {
private:
    Node* m_root;
    int REST_NB, REST_NR, REST_NE; //REST_NB : 残りの青駒の数, REST_NR : 残りの赤駒の数, REST_EB : 残りの敵駒の数
    ZDD() noexcept; // コンストラクタを private に置く。
    ZDD(int i, int j, int k) noexcept; // コンストラクタを private に置く。
    ZDD(const ZDD&); // コピーコンストラクタも private に置き、定義しない。
    ZDD& operator=(const ZDD&); // コピー代入演算子も private に置き、定義しない。
    ~ZDD() noexcept; // デストラクタを private に置く。
    
public:
    static ZDD& get(int i, int j, int k, int l) noexcept;
    static ZDD& get(int iter, int i, int j, int k, int l) noexcept;
    void out_info() const noexcept;
    void compute_array(unsigned long long int id, unsigned char array_sq[36]) const noexcept;  //整数値から配置を求める関数
    unsigned long long int compute_id(unsigned char array_sq[36]) const noexcept;  // 配置から整数値を求める関数
    unsigned long long int compute_id_reverse(unsigned char array_sq[36]) const noexcept;  // 配置を反転させたときの整数値を得る(gened_L.cppで使う)
    int compute_length(unsigned long long int id) const noexcept;
    unsigned long long int get_path_num() const noexcept {
        // assert(m_root->num == 12352692569ULL);  
        return m_root->num;
    }
    void print_board(unsigned long long int id) const noexcept;
    unsigned long long int get_num() const noexcept { return m_root->num; };
    int get_rest_num(unsigned int kind_piece) const noexcept;
};
#endif
