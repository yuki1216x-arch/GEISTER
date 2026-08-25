#!/usr/bin/env bash
function gen_ed()
{
    date
    for iter in {1..100}
    do
        ./bin/gened "$iter" $1 $2 $3 $4 | tee gened1.log 2>&1
        date
        if [ $1 -ne $3 ] || [ $2 -ne $4 ]; then
            ./bin/gened "$iter" $3 $4 $1 $2 | tee gened2.log 2>&1
            date
        fi
        ./bin/gened_L "$iter" $1 $2 $3 $4
        date
        if [ $1 -ne $3 ] || [ $2 -ne $4 ]; then
            ./bin/gened_L "$iter" $3 $4 $1 $2 2>&1
            date
        fi
        if grep "no changed this rupe!!" gened1.log > /dev/null; then
            echo "program is finished"
            break
        fi
        if [ $1 -ne $3 ] || [ $2 -ne $4 ]; then
            if grep "no changed this rupe!!" gened1.log > /dev/null && grep "no changed this rupe!!" gened2.log > /dev/null; then
                echo "program is finished"
                break
            fi
        fi
    done
}

function gen_ed_canwin()
{
    date
    for iter in {1..100}
    do
        ./bin/gened_can-win "$iter" $1 $2 $3 $4 | tee gened1.log 2>&1
        date
        if grep "no changed this rupe!!" gened1.log > /dev/null; then
            echo "program is finished"
            break
        fi
    done
}

# 残り駒4
gen_ed 1 1 1 1 > data/output/database1-1-1-1.txt

# 勝ち有りのやつ
gen_ed_canwin 1 1 1 1 > data/output/database_cw1-1-1-1.txt


# 残り駒5
gen_ed 1 1 1 2 > data/output/database1-1-1-2.txt
gen_ed 1 1 2 1 > data/output/database1-1-2-1.txt

# 勝ち有りのやつ
gen_ed_canwin 1 1 1 2 > data/output/database_cw1-1-1-2.txt
gen_ed_canwin 1 1 2 1 > data/output/database_cw1-1-2-1.txt
gen_ed_canwin 1 2 1 1 > data/output/database_cw1-2-1-1.txt
gen_ed_canwin 2 1 1 1 > data/output/database_cw2-1-1-1.txt


# 残り駒6
gen_ed 1 1 1 3 > data/output/database1-1-1-3.txt
gen_ed 1 1 2 2 > data/output/database1-1-2-2.txt
gen_ed 1 1 3 1 > data/output/database1-1-3-1.txt
gen_ed 1 2 1 2 > data/output/database1-2-1-2.txt
gen_ed 1 2 2 1 > data/output/database1-2-2-1.txt
gen_ed 2 1 2 1 > data/output/database2-1-2-1.txt

# 勝ち有りのやつ
gen_ed_canwin 1 1 1 3 > data/output/database_cw1-1-1-3.txt
gen_ed_canwin 1 1 2 2 > data/output/database_cw1-1-2-2.txt
gen_ed_canwin 1 1 3 1 > data/output/database_cw1-1-3-1.txt
gen_ed_canwin 1 2 1 2 > data/output/database_cw1-2-1-2.txt
gen_ed_canwin 1 2 2 1 > data/output/database_cw1-2-2-1.txt
gen_ed_canwin 1 3 1 1 > data/output/database_cw1-3-1-1.txt
gen_ed_canwin 2 1 1 2 > data/output/database_cw2-1-1-2.txt
gen_ed_canwin 2 1 2 1 > data/output/database_cw2-1-2-1.txt
gen_ed_canwin 2 2 1 1 > data/output/database_cw2-2-1-1.txt
gen_ed_canwin 3 1 1 1 > data/output/database_cw3-1-1-1.txt


# 残り駒7
gen_ed 1 1 1 4 > data/output/database1-1-1-4.txt
gen_ed 1 1 2 3 > data/output/database1-1-2-3.txt
gen_ed 1 1 3 2 > data/output/database1-1-3-2.txt
gen_ed 1 1 4 1 > data/output/database1-1-4-1.txt
gen_ed 1 2 1 3 > data/output/database1-2-1-3.txt
gen_ed 1 2 2 2 > data/output/database1-2-2-2.txt
gen_ed 1 2 3 1 > data/output/database1-2-3-1.txt
gen_ed 1 3 2 1 > data/output/database1-3-2-1.txt
gen_ed 2 1 2 2 > data/output/database2-1-2-2.txt
gen_ed 2 1 3 1 > data/output/database2-1-3-1.txt

# 勝ち有りのやつ
gen_ed_canwin 1 1 1 4 > data/output/database_cw1-1-1-4.txt
gen_ed_canwin 1 1 2 3 > data/output/database_cw1-1-2-3.txt
gen_ed_canwin 1 1 3 2 > data/output/database_cw1-1-3-2.txt
gen_ed_canwin 1 1 4 1 > data/output/database_cw1-1-4-1.txt
gen_ed_canwin 1 2 1 3 > data/output/database_cw1-2-1-3.txt
gen_ed_canwin 1 2 2 2 > data/output/database_cw1-2-2-2.txt
gen_ed_canwin 1 2 3 1 > data/output/database_cw1-2-3-1.txt
gen_ed_canwin 1 3 1 2 > data/output/database_cw1-3-1-2.txt
gen_ed_canwin 1 3 2 1 > data/output/database_cw1-3-2-1.txt
gen_ed_canwin 1 4 1 1 > data/output/database_cw1-4-1-1.txt
gen_ed_canwin 2 1 1 3 > data/output/database_cw2-1-1-3.txt
gen_ed_canwin 2 1 2 2 > data/output/database_cw2-1-2-2.txt
gen_ed_canwin 2 1 3 1 > data/output/database_cw2-1-3-1.txt
gen_ed_canwin 2 2 1 2 > data/output/database_cw2-2-1-2.txt
gen_ed_canwin 2 2 2 1 > data/output/database_cw2-2-2-1.txt
gen_ed_canwin 2 3 1 1 > data/output/database_cw2-3-1-1.txt
gen_ed_canwin 3 1 1 2 > data/output/database_cw3-1-1-2.txt
gen_ed_canwin 3 1 2 1 > data/output/database_cw3-1-2-1.txt
gen_ed_canwin 3 2 1 1 > data/output/database_cw3-2-1-1.txt
gen_ed_canwin 4 1 1 1 > data/output/database_cw4-1-1-1.txt


# 残り駒8
gen_ed 1 1 2 4 > data/output/database1-1-2-4.txt
gen_ed 1 1 3 3 > data/output/database1-1-3-3.txt
gen_ed 1 1 4 2 > data/output/database1-1-4-2.txt
gen_ed 1 2 1 4 > data/output/database1-2-1-4.txt
gen_ed 1 2 2 3 > data/output/database1-2-2-3.txt
gen_ed 1 2 3 2 > data/output/database1-2-3-2.txt
gen_ed 1 2 4 1 > data/output/database1-2-4-1.txt
gen_ed 1 3 1 3 > data/output/database1-3-1-3.txt
gen_ed 1 3 2 2 > data/output/database1-3-2-2.txt
gen_ed 1 3 3 1 > data/output/database1-3-3-1.txt
gen_ed 1 4 2 1 > data/output/database1-4-2-1.txt
gen_ed 2 1 2 3 > data/output/database2-1-2-3.txt
gen_ed 2 1 3 2 > data/output/database2-1-3-2.txt
gen_ed 2 1 4 1 > data/output/database2-1-4-1.txt
gen_ed 2 2 2 2 > data/output/database2-2-2-2.txt
gen_ed 2 2 3 1 > data/output/database2-2-3-1.txt
gen_ed 3 1 3 1 > data/output/database3-1-3-1.txt

# 勝ち有りのやつ
gen_ed_canwin 1 1 2 4 > data/output/database_cw1-1-2-4.txt
gen_ed_canwin 1 1 3 3 > data/output/database_cw1-1-3-3.txt
gen_ed_canwin 1 1 4 2 > data/output/database_cw1-1-4-2.txt
gen_ed_canwin 1 2 1 4 > data/output/database_cw1-2-1-4.txt
gen_ed_canwin 1 2 2 3 > data/output/database_cw1-2-2-3.txt
gen_ed_canwin 1 2 3 2 > data/output/database_cw1-2-3-2.txt
gen_ed_canwin 1 2 4 1 > data/output/database_cw1-2-4-1.txt
gen_ed_canwin 1 3 1 3 > data/output/database_cw1-3-1-3.txt
gen_ed_canwin 1 3 2 2 > data/output/database_cw1-3-2-2.txt
gen_ed_canwin 1 3 3 1 > data/output/database_cw1-3-3-1.txt
gen_ed_canwin 1 4 1 2 > data/output/database_cw1-4-1-2.txt
gen_ed_canwin 1 4 2 1 > data/output/database_cw1-4-2-1.txt
gen_ed_canwin 2 1 1 4 > data/output/database_cw2-1-1-4.txt
gen_ed_canwin 2 1 2 3 > data/output/database_cw2-1-2-3.txt
gen_ed_canwin 2 1 3 2 > data/output/database_cw2-1-3-2.txt
gen_ed_canwin 2 1 4 1 > data/output/database_cw2-1-4-1.txt
gen_ed_canwin 2 2 1 3 > data/output/database_cw2-2-1-3.txt
gen_ed_canwin 2 2 2 2 > data/output/database_cw2-2-2-2.txt
gen_ed_canwin 2 2 3 1 > data/output/database_cw2-2-3-1.txt
gen_ed_canwin 2 3 1 2 > data/output/database_cw2-3-1-2.txt
gen_ed_canwin 2 3 2 1 > data/output/database_cw2-3-2-1.txt
gen_ed_canwin 2 4 1 1 > data/output/database_cw2-4-1-1.txt
gen_ed_canwin 3 1 1 3 > data/output/database_cw3-1-1-3.txt
gen_ed_canwin 3 1 2 2 > data/output/database_cw3-1-2-2.txt
gen_ed_canwin 3 1 3 1 > data/output/database_cw3-1-3-1.txt
gen_ed_canwin 3 2 1 2 > data/output/database_cw3-2-1-2.txt
gen_ed_canwin 3 2 2 1 > data/output/database_cw3-2-2-1.txt
gen_ed_canwin 3 3 1 1 > data/output/database_cw3-3-1-1.txt
gen_ed_canwin 4 1 1 2 > data/output/database_cw4-1-1-2.txt
gen_ed_canwin 4 1 2 1 > data/output/database_cw4-1-2-1.txt
gen_ed_canwin 4 2 1 1 > data/output/database_cw4-2-1-1.txt
