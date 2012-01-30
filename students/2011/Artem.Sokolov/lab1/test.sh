#!/bin/bash

echo "Generaing AST representations..."
mkdir tests

for f in $(ls ../../../../tests/*.mvm)
do 
	echo "--------------------------------------------------------------------------------"
    echo "Generating AST from" $f""
    build/debug/lab1 $f > tests/$(basename $f)
    echo "==================== Compare original: ===================="
    cat $f
    echo
    echo "===================== and generated: ====================="    
    build/debug/lab1 tests/$(basename $f)
    #cat tests/$(basename $f)
    echo
done

for f in $(ls ../../../../tests/optional/*.mvm)
do 
	echo "--------------------------------------------------------------------------------"
    echo "Generating AST from" $f""
    build/debug/lab1 $f > tests/$(basename $f)
    echo "==================== Compare original: ===================="
    cat $f
    echo
    echo "===================== and generated: ====================="    
    build/debug/lab1 tests/$(basename $f)
    #cat tests/$(basename $f)
    echo
done

rm -rf tests
