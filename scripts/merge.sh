#!/bin/sh
function merge_ed()
{
    # 残り駒4
    # date
    # ./merge_db ./table/table1-1-1-1_p1.bin ./table/table_cw1-1-1-1_p1.bin 706860
    # 手数を考慮したいとき
    # ./db/db1-1-1-1_p1.bin

    # 残り駒5
    # date
    # ./merge_db ./table/table1-1-1-2_p1.bin ./table/table_cw1-1-1-2_p1.bin 7539840
    # date
    # ./merge_db ./table/table1-1-2-1_p1.bin ./table/table_cw1-1-2-1_p1.bin 7539840
    # date
    # ./merge_db ./table/table1-2-1-1_p1.bin ./table/table_cw1-2-1-1_p1.bin 11309760
    # date
    # ./merge_db ./table/table2-1-1-1_p1.bin ./table/table_cw2-1-1-1_p1.bin 11309760

    # 残り駒6
    # date
    # ./merge_db ./table/table1-1-1-3_p1.bin ./table/table_cw1-1-1-3_p1.bin 58433760
    # date
    # ./merge_db ./table/table1-1-2-2_p1.bin ./table/table_cw1-1-2-2_p1.bin 58433760
    # date
    # ./merge_db ./table/table1-1-3-1_p1.bin ./table/table_cw1-1-3-1_p1.bin 58433760
    # date
    # ./merge_db ./table/table1-2-1-2_p1.bin ./table/table_cw1-2-1-2_p1.bin 116867520
    # date
    # ./merge_db ./table/table1-2-2-1_p1.bin ./table/table_cw1-2-2-1_p1.bin 116867520
    # date
    # ./merge_db ./table/table1-3-1-1_p1.bin ./table/table_cw1-3-1-1_p1.bin 116867520
    # date
    # ./merge_db ./table/table2-1-1-2_p1.bin ./table/table_cw2-1-1-2_p1.bin 116867520
    # date
    # ./merge_db ./table/table2-1-2-1_p1.bin ./table/table_cw2-1-2-1_p1.bin 116867520
    # date
    # ./merge_db ./table/table2-2-1-1_p1.bin ./table/table_cw2-2-1-1_p1.bin 175301280
    # date
    # ./merge_db ./table/table3-1-1-1_p1.bin ./table/table_cw3-1-1-1_p1.bin 116867520

    # 残り駒7
    # date
    # ./merge_db ./table/table1-1-1-4_p1.bin ./table/table_cw1-1-1-4_p1.bin 350602560
    # date
    # ./merge_db ./table/table1-1-2-3_p1.bin ./table/table_cw1-1-2-3_p1.bin 350602560
    # date
    # ./merge_db ./table/table1-1-3-2_p1.bin ./table/table_cw1-1-3-2_p1.bin 350602560
    # date
    # ./merge_db ./table/table1-1-4-1_p1.bin ./table/table_cw1-1-4-1_p1.bin 350602560
    # date
    # ./merge_db ./table/table1-2-1-3_p1.bin ./table/table_cw1-2-1-3_p1.bin 876506400
    # date
    # ./merge_db ./table/table1-2-2-2_p1.bin ./table/table_cw1-2-2-2_p1.bin 876506400
    # date
    # ./merge_db ./table/table1-2-3-1_p1.bin ./table/table_cw1-2-3-1_p1.bin 876506400
    # date
    # ./merge_db ./table/table1-3-1-2_p1.bin ./table/table_cw1-3-1-2_p1.bin 1168675200
    # date
    # ./merge_db ./table/table1-3-2-1_p1.bin ./table/table_cw1-3-2-1_p1.bin 1168675200
    # date
    # ./merge_db ./table/table1-4-1-1_p1.bin ./table/table_cw1-4-1-1_p1.bin 876506400
    # date
    # ./merge_db ./table/table2-1-1-3_p1.bin ./table/table_cw2-1-1-3_p1.bin 876506400
    # date
    # ./merge_db ./table/table2-1-2-2_p1.bin ./table/table_cw2-1-2-2_p1.bin 876506400
    # date
    # ./merge_db ./table/table2-1-3-1_p1.bin ./table/table_cw2-1-3-1_p1.bin 876506400
    date
    ./merge_db ./table/table2-2-1-2_p1.bin ./table/table_cw2-2-1-2_p1.bin 1753012800
    # date
    # ./merge_db ./table/table2-2-2-1_p1.bin ./table/table_cw2-2-2-1_p1.bin 1753012800
    # date
    # ./merge_db ./table/table2-3-1-1_p1.bin ./table/table_cw2-3-1-1_p1.bin 1753012800
    # date
    # ./merge_db ./table/table3-1-1-2_p1.bin ./table/table_cw3-1-1-2_p1.bin 1168675200
    # date
    # ./merge_db ./table/table3-1-2-1_p1.bin ./table/table_cw3-1-2-1_p1.bin 1168675200
    # date
    # ./merge_db ./table/table3-2-1-1_p1.bin ./table/table_cw3-2-1-1_p1.bin 1753012800
    # date
    # ./merge_db ./table/table4-1-1-1_p1.bin ./table/table_cw4-1-1-1_p1.bin 876506400
    # date
}

merge_ed | tee test1.txt
# merge_ed | tee test1-2-1-2.txt
# merge_ed | tee merge_databases.txt
