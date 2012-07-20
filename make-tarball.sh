#!/bin/bash

cd "$(dirname "$0")"

set -ex

make -C solver clean
make -C solver all

tar cvzhf ./icfp-96697646.tgz -C submission --exclude output.tmp .
