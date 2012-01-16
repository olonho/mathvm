#!/bin/bash

echo "Generaing AST representations..."
mkdir tests

for f in $(ls ../../../../tests/*.mvm)
do 
    echo "Generating AST from" $f""
    build/debug/lab3 $f > tests/$(basename $f)
    echo "==== Compare original: ===="
    cat $f
    echo
    echo "==== and generated: ====="    
    cat tests/$(basename $f)
    echo
done

rm -rf tests
