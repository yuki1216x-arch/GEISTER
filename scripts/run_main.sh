#!/usr/bin/env bash

# 4
./bin/main 0 1 1 1 1 > data/output/make1-1-1-1.txt 2>&1 &
wait

# 5
./bin/main 0 1 1 1 2 > data/output/make1-1-1-2.txt 2>&1 &
./bin/main 0 1 1 2 1 > data/output/make1-1-2-1.txt 2>&1 &
wait

# 6
./bin/main 0 1 1 1 3 > data/output/make1-1-1-3.txt 2>&1 &
./bin/main 0 1 1 2 2 > data/output/make1-1-2-2.txt 2>&1 &
./bin/main 0 1 1 3 1 > data/output/make1-1-3-1.txt 2>&1 &
./bin/main 0 1 2 1 2 > data/output/make1-2-1-2.txt 2>&1 &
./bin/main 0 1 2 2 1 > data/output/make1-2-2-1.txt 2>&1 &
./bin/main 0 2 1 2 1 > data/output/make2-1-2-1.txt 2>&1 &
wait

# 7
./bin/main 0 1 1 1 4 > data/output/make1-1-1-4.txt 2>&1 &
./bin/main 0 1 1 2 3 > data/output/make1-1-2-3.txt 2>&1 &
./bin/main 0 1 1 3 2 > data/output/make1-1-3-2.txt 2>&1 &
./bin/main 0 1 1 4 1 > data/output/make1-1-4-1.txt 2>&1 &
./bin/main 0 1 2 1 3 > data/output/make1-2-1-3.txt 2>&1 &
wait
./bin/main 0 1 2 2 2 > data/output/make1-2-2-2.txt 2>&1 &
./bin/main 0 1 2 3 1 > data/output/make1-2-3-1.txt 2>&1 &
./bin/main 0 1 3 2 1 > data/output/make1-3-2-1.txt 2>&1 &
./bin/main 0 2 1 2 2 > data/output/make2-1-2-2.txt 2>&1 &
./bin/main 0 2 1 3 1 > data/output/make2-1-3-1.txt 2>&1 &
wait

# 8
./bin/main 0 1 1 2 4 > data/output/make1-1-2-4.txt 2>&1 &
./bin/main 0 1 1 3 3 > data/output/make1-1-3-3.txt 2>&1 &
./bin/main 0 1 1 4 2 > data/output/make1-1-4-2.txt 2>&1 &
./bin/main 0 1 2 1 4 > data/output/make1-2-1-4.txt 2>&1 &
wait
./bin/main 0 1 2 2 3 > data/output/make1-2-2-3.txt 2>&1 &
./bin/main 0 1 2 3 2 > data/output/make1-2-3-2.txt 2>&1 &
wait
./bin/main 0 1 2 4 1 > data/output/make1-2-4-1.txt 2>&1 &
./bin/main 0 1 3 1 3 > data/output/make1-3-1-3.txt 2>&1 &
./bin/main 0 1 3 2 2 > data/output/make1-3-2-2.txt 2>&1 &
./bin/main 0 1 3 3 1 > data/output/make1-3-3-1.txt 2>&1 &
wait
./bin/main 0 1 4 2 1 > data/output/make1-4-2-1.txt 2>&1 &
./bin/main 0 2 1 2 3 > data/output/make2-1-2-3.txt 2>&1 &
./bin/main 0 2 1 3 2 > data/output/make2-1-3-2.txt 2>&1 &
wait
./bin/main 0 2 1 4 1 > data/output/make2-1-4-1.txt 2>&1 &
./bin/main 0 2 2 2 2 > data/output/make2-2-2-2.txt 2>&1 &
./bin/main 0 2 2 3 1 > data/output/make2-2-3-1.txt 2>&1 &
./bin/main 0 3 1 3 1 > data/output/make3-1-3-1.txt 2>&1 &
wait
