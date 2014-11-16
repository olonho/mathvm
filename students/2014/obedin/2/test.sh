#!/usr/bin/env bash

ROOT_DIR=$HOME/Projects/playground/mathvm
TESTS_DIR=$ROOT_DIR/tests
MVM=$ROOT_DIR/students/2014/obedin/2/build/debug/mvm

rm -rf out/*
mkdir -p out
for TEST_DIR in $TESTS_DIR; do
    for TEST in `find $TEST_DIR -name '*.mvm'`; do
        echo $TEST
        OUT="out/`basename $TEST`.txt"
        $MVM $TEST >>$OUT
        cp $TEST out/`basename $TEST`
    done
done
