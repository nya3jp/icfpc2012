#!/bin/bash

exec 2> /dev/null

cd "$(dirname "$0")/.."

mapfile=maps/contest2.map
command_size=40

while :; do
  move=$(tools/random_command.py $command_size)
  ref1=$(echo $move | python/console.py $mapfile)
  ref2=$(solver/simple_simulator $mapfile $move | grep Score | awk '{print $5}')
  ref3=$(solver/console $mapfile $move | tail -n 1 | awk '{print $2}')
  echo "$move: $ref1 $ref2 $ref3"
  if [[ "z$ref1" != "z$ref2" ]] || [[ "z$ref2" != "z$ref3" ]]; then
    cat <<EOF
＿人人 人人＿ 
＞ 突然の死 ＜ 
￣Y^Y^Y^Y￣
EOF
  fi
done
