#!/bin/bash

cd "$(dirname "$0")"

set -ex

./build_test.sh
./simulator_test.sh
