#!/bin/bash

mkdir -p tests
mkdir -p tests2

for file in $(ls ../../../../tests/*.mvm)
do 
    build/debug/lab1 $file tests/$(basename $file)
    build/debug/lab1 tests/$(basename $file) tests2/$(basename $file)
    
    cmp  tests/$(basename $file) tests2/$(basename $file)
done

rm -rf tests tests2
