#!/usr/bin/env bash

# 4
./bin/main_purple 1 1 1 1 1 > data/output/purple/make1-1-1-1.txt 2>&1 &
wait

# 5
./bin/main_purple 1 1 1 1 2 > data/output/purple/make1-1-1-2.txt 2>&1 &
./bin/main_purple 1 1 1 2 1 > data/output/purple/make1-1-2-1.txt 2>&1 &
./bin/main_purple 1 1 2 1 1 > data/output/purple/make1-2-1-1.txt 2>&1 &
./bin/main_purple 1 2 1 1 1 > data/output/purple/make2-1-1-1.txt 2>&1 &
wait

# 6
./bin/main_purple 1 1 1 1 3 > data/output/purple/make1-1-1-3.txt 2>&1 &
./bin/main_purple 1 1 1 2 2 > data/output/purple/make1-1-2-2.txt 2>&1 &
./bin/main_purple 1 1 1 3 1 > data/output/purple/make1-1-3-1.txt 2>&1 &
./bin/main_purple 1 1 2 1 2 > data/output/purple/make1-2-1-2.txt 2>&1 &
./bin/main_purple 1 1 2 2 1 > data/output/purple/make1-2-2-1.txt 2>&1 &
wait
./bin/main_purple 1 1 3 1 1 > data/output/purple/make1-3-1-1.txt 2>&1 &
./bin/main_purple 1 2 1 1 2 > data/output/purple/make2-1-1-2.txt 2>&1 &
./bin/main_purple 1 2 1 2 1 > data/output/purple/make2-1-2-1.txt 2>&1 &
./bin/main_purple 1 2 2 1 1 > data/output/purple/make2-2-1-1.txt 2>&1 &
./bin/main_purple 1 3 1 1 1 > data/output/purple/make3-1-1-1.txt 2>&1 &
wait

# 7
./bin/main_purple 1 1 1 1 4 > data/output/purple/make1-1-1-4.txt 2>&1 &
./bin/main_purple 1 1 1 2 3 > data/output/purple/make1-1-2-3.txt 2>&1 &
./bin/main_purple 1 1 1 3 2 > data/output/purple/make1-1-3-2.txt 2>&1 &
./bin/main_purple 1 1 1 4 1 > data/output/purple/make1-1-4-1.txt 2>&1 &
./bin/main_purple 1 1 2 1 3 > data/output/purple/make1-2-1-3.txt 2>&1 &
wait
./bin/main_purple 1 1 2 2 2 > data/output/purple/make1-2-2-2.txt 2>&1 &
./bin/main_purple 1 1 2 3 1 > data/output/purple/make1-2-3-1.txt 2>&1 &
./bin/main_purple 1 1 3 1 2 > data/output/purple/make1-3-1-2.txt 2>&1 &
./bin/main_purple 1 1 3 2 1 > data/output/purple/make1-3-2-1.txt 2>&1 &
./bin/main_purple 1 1 4 1 1 > data/output/purple/make1-4-1-1.txt 2>&1 &
wait
./bin/main_purple 1 2 1 1 3 > data/output/purple/make2-1-1-3.txt 2>&1 &
./bin/main_purple 1 2 1 2 2 > data/output/purple/make2-1-2-2.txt 2>&1 &
./bin/main_purple 1 2 1 3 1 > data/output/purple/make2-1-3-1.txt 2>&1 &
./bin/main_purple 1 2 2 1 2 > data/output/purple/make2-2-1-2.txt 2>&1 &
./bin/main_purple 1 2 2 2 1 > data/output/purple/make2-2-2-1.txt 2>&1 &
wait
./bin/main_purple 1 2 3 1 1 > data/output/purple/make2-3-1-1.txt 2>&1 &
./bin/main_purple 1 3 1 1 2 > data/output/purple/make3-1-1-2.txt 2>&1 &
./bin/main_purple 1 3 1 2 1 > data/output/purple/make3-1-2-1.txt 2>&1 &
./bin/main_purple 1 3 2 1 1 > data/output/purple/make3-2-1-1.txt 2>&1 &
./bin/main_purple 1 4 1 1 1 > data/output/purple/make4-1-1-1.txt 2>&1 &
wait

# 8
./bin/main_purple 1 1 1 2 4 > data/output/purple/make1-1-2-4.txt 2>&1 &
./bin/main_purple 1 1 1 3 3 > data/output/purple/make1-1-3-3.txt 2>&1 &
./bin/main_purple 1 1 1 4 2 > data/output/purple/make1-1-4-2.txt 2>&1 &
./bin/main_purple 1 1 2 1 4 > data/output/purple/make1-2-1-4.txt 2>&1 &
./bin/main_purple 1 1 2 2 3 > data/output/purple/make1-2-2-3.txt 2>&1 &
wait
./bin/main_purple 1 1 2 3 2 > data/output/purple/make1-2-3-2.txt 2>&1 &
./bin/main_purple 1 1 2 4 1 > data/output/purple/make1-2-4-1.txt 2>&1 &
./bin/main_purple 1 1 3 1 3 > data/output/purple/make1-3-1-3.txt 2>&1 &
./bin/main_purple 1 1 3 2 2 > data/output/purple/make1-3-2-2.txt 2>&1 &
./bin/main_purple 1 1 3 3 1 > data/output/purple/make1-3-3-1.txt 2>&1 &
wait
./bin/main_purple 1 1 4 1 2 > data/output/purple/make1-4-1-2.txt 2>&1 &
./bin/main_purple 1 1 4 2 1 > data/output/purple/make1-4-2-1.txt 2>&1 &
./bin/main_purple 1 2 1 1 4 > data/output/purple/make2-1-1-4.txt 2>&1 &
./bin/main_purple 1 2 1 2 3 > data/output/purple/make2-1-2-3.txt 2>&1 &
./bin/main_purple 1 2 1 3 2 > data/output/purple/make2-1-3-2.txt 2>&1 &
wait
./bin/main_purple 1 2 1 4 1 > data/output/purple/make2-1-4-1.txt 2>&1 &
./bin/main_purple 1 2 2 1 3 > data/output/purple/make2-2-1-3.txt 2>&1 &
./bin/main_purple 1 2 2 2 2 > data/output/purple/make2-2-2-2.txt 2>&1 &
./bin/main_purple 1 2 2 3 1 > data/output/purple/make2-2-3-1.txt 2>&1 &
./bin/main_purple 1 2 3 1 2 > data/output/purple/make2-3-1-2.txt 2>&1 &
wait
./bin/main_purple 1 2 3 2 1 > data/output/purple/make2-3-2-1.txt 2>&1 &
./bin/main_purple 1 2 4 1 1 > data/output/purple/make2-4-1-1.txt 2>&1 &
./bin/main_purple 1 3 1 1 3 > data/output/purple/make3-1-1-3.txt 2>&1 &
./bin/main_purple 1 3 1 2 2 > data/output/purple/make3-1-2-2.txt 2>&1 &
./bin/main_purple 1 3 1 3 1 > data/output/purple/make3-1-3-1.txt 2>&1 &
wait
./bin/main_purple 1 3 2 1 2 > data/output/purple/make3-2-1-2.txt 2>&1 &
./bin/main_purple 1 3 2 2 1 > data/output/purple/make3-2-2-1.txt 2>&1 &
./bin/main_purple 1 3 3 1 1 > data/output/purple/make3-3-1-1.txt 2>&1 &
./bin/main_purple 1 4 1 1 2 > data/output/purple/make4-1-1-2.txt 2>&1 &
./bin/main_purple 1 4 1 2 1 > data/output/purple/make4-1-2-1.txt 2>&1 &
wait
./bin/main_purple 1 4 2 1 1 > data/output/purple/make4-2-1-1.txt 2>&1 &
wait

