#!/bin/bash

set -e

cd "$(dirname "$0")/../solver"

make clean
make
