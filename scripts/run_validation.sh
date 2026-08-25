#!/usr/bin/env bash

# 4
./bin/validation 1 1 1 1 > data/output/validation/validation1-1-1-1.txt 2>&1

# 5
./bin/validation 1 1 1 2 > data/output/validation/validation1-1-1-2.txt 2>&1
./bin/validation 1 1 2 1 > data/output/validation/validation1-1-2-1.txt 2>&1
./bin/validation 1 2 1 1 > data/output/validation/validation1-2-1-1.txt 2>&1
./bin/validation 2 1 1 1 > data/output/validation/validation2-1-1-1.txt 2>&1

# 6
./bin/validation 1 1 1 3 > data/output/validation/validation1-1-1-3.txt 2>&1
./bin/validation 1 1 2 2 > data/output/validation/validation1-1-2-2.txt 2>&1
./bin/validation 1 1 3 1 > data/output/validation/validation1-1-3-1.txt 2>&1
./bin/validation 1 2 1 2 > data/output/validation/validation1-2-1-2.txt 2>&1
./bin/validation 1 2 2 1 > data/output/validation/validation1-2-2-1.txt 2>&1
./bin/validation 1 3 1 1 > data/output/validation/validation1-3-1-1.txt 2>&1
./bin/validation 2 1 1 2 > data/output/validation/validation2-1-1-2.txt 2>&1
./bin/validation 2 1 2 1 > data/output/validation/validation2-1-2-1.txt 2>&1
./bin/validation 2 2 1 1 > data/output/validation/validation2-2-1-1.txt 2>&1
./bin/validation 3 1 1 1 > data/output/validation/validation3-1-1-1.txt 2>&1
