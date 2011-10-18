#!/bin/bash

#CW=`pwd`
#cd build/bin
#ln -s lab1 mvm
#cd $CW

echo "Generaing AST representations..."
mkdir tests
for f in $(ls ../../../../tests/*mvm); do echo "Generating AST from" $f": "; build/bin/lab1 $f > tests/$(basename $f); echo; done
cp ../../../../tests/*expect tests/

echo "Checking generated representations..."

cd tests
for f in $(ls *mvm); do echo "Checking" $f": "; ../build/bin/lab1 $f; echo; done

#../../../../tests/run.py

cd ..
rm -rf tests

#for f in $(ls ../../../../tests/*mvm); do echo "Running test using " $f": "; build/bin/lab1 $f; echo; done