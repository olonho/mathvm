#!/bin/bash

echo "Generaing AST representations..."
for f in $(ls ../../../../tests/*mvm); do echo "Generating AST from" $f": "; build/bin/lab1 $f > $(basename $f); echo; done

echo "Checking generated representation..."

for f in $(ls *mvm); do echo "Checking" $f": "; build/bin/lab1 $f; echo; done
rm *mvm

#for f in $(ls ../../../../tests/*mvm); do echo "Running test using " $f": "; build/bin/lab1 $f; echo; done