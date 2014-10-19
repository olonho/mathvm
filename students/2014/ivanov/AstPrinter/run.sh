#!/bin/bash

TEST_DIR=../../../../tests/*.mvm
APP=./build/debug/mvm
for f in $TEST_DIR
do
  echo $f
  ${APP} $f
  echo "============================================"
done

