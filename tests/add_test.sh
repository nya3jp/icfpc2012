#!/bin/bash

if [[ "$#" != 2 ]]; then
  echo "usage: $0 <mapfile> <move>"
  exit 1
fi

if [[ ! -f "$1" ]]; then
  echo "can't find $1"
  exit 1
fi

set -e

make -C "$(dirname "$0")/../solver" simple_simulator
echo
echo

f="$(dirname "$0")/testdata/$(basename "$1" | sed 's/\..*$//')"

for i in `seq -f %04g 0 9999`; do
  if [[ ! -f "$f-$i.test" ]]; then
    break
  fi
done
test_file="$f-$i.test"
golden_file="$f-$i.golden"

echo "$2" | cat - "$1" > $test_file

"$(dirname "$0")/../solver/simple_simulator" $1 $2 | tee $golden_file

echo "generated $(basename "$1" | sed 's/\..*$//')-$i.{test,golden}"
