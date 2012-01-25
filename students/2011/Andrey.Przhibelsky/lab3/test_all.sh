#!/bin/bash
mkdir -p tests

for file in $(ls ../../../../tests/*.mvm)
do 
	build/debug/lab2 $file 
done

rm -rf tests
