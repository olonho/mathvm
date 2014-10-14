#!/bin/bash

testDir=$1
firstResult=first_result.txt
secondResult=second_result.txt

rm -f $firstResult $secondResult


for testFile in $(find $testDir -type f -name "*.mvm")
do
  build/debug/mvm $testFile > $firstResult
  build/debug/mvm $firstResult > $secondResult
  diff $firstResult $secondResult
  if [[ $? != 0 ]]; then
    echo "Failed on $testFile"
  fi
  rm -f $firstResult $secondResult
done

