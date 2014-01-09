#!/bin/bash

function testcase {
  echo "______________________________________"
  echo "test: " $1
  ./build/debug/mvm ../../../../tests/$1.mvm > output
  cat output
  echo "-----------------cmp------------------"
  diff output ../../../../tests/$1.expect
  echo "--------------------------------------"
  echo ""
}

for test in add assign bitwise div expr for function if literal mul sub while; do
  testcase $test
done

rm output
