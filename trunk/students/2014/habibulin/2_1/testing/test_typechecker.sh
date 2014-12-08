#!/bin/bash

echo ""
FILES=./mytests/*.mvm
echo "Testing $FILES"
for f in $FILES
do
  echo "Testing $f"
  ../build/debug/mvm $f
done 

echo ""
FILES=./mytests/additional/*.mvm
echo "Testing $FILES"
for f in $FILES
do
  echo "Testing $f"
  ../build/debug/mvm $f
done 

echo ""
FILES=./mytests/additional/fail/*.mvm
echo "Testing $FILES"
for f in $FILES
do
  echo "Testing $f"
  ../build/debug/mvm $f
done 
