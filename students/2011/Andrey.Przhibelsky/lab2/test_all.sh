#!/bin/bash

for file in $(ls ../../../../tests/*.mvm)
do 
    build/debug/lab2 $file 
done


