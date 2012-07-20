#!/bin/bash

base_dir="$(dirname "$0")"

make -C "$base_dir/solver" all

time_limit=5
if [[ $# = 0 ]]; then
  mapfiles=($base_dir/maps/*.map)
else
  mapfiles=("$@")
fi

$base_dir/tools/evaluate_solution.py -p $base_dir/submission/lifter -H $base_dir/maps/HIGHSCORES -d -a "--debug --fixed-time-limit $time_limit --detail-output-dir=$base_dir/maps --input-name=%mapname%" "${mapfiles[@]}"
