#!/bin/bash

set -e

cd "$(dirname "$0")"

make -C ../solver simple_simulator

tmp_file=/tmp/golden_test.$USER.txt

for test_file in testdata/*.test; do
  echo "testing: $test_file"
  golden_file=${test_file%%.test}.golden
  move=$(head -n 1 $test_file)
  sed 1d $test_file > $tmp_file
  ../solver/simple_simulator $tmp_file $move | diff -u $golden_file -
done
