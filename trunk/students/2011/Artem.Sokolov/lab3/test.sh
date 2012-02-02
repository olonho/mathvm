#!/bin/bash
mkdir -p tests

for file in $(ls ../../../../tests/*.mvm)
do 
	filename=$(basename $file)
	build/debug/lab3 $file tests/${filename%.*}.expect

	cmp ../../../../tests/${filename%.*}.expect tests/${filename%.*}.expect
done

rm -rf tests
