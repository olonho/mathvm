#!/bin/bash

MATH_VM=../../../..

MATH_VM_TESTS=${MATH_VM}/tests
MATH_VM_BIN=./build/opt/mvm

SIMPLE_TESTS=${MATH_VM_TESTS}/.
OPTIONAL=${MATH_VM_TESTS}/optional
ADDITIONAL=${MATH_VM_TESTS}/additional
FAIL=${MATH_VM_TESTS}/additional/fail
PERF=${MATH_VM_TESTS}/perf
MY_TESTS=./mytests

function run_test {
    echo -n "test:" ${1##*/} "-- "
    bench=$( { time ${MATH_VM_BIN} ${1}.mvm > output; } 2>&1 )
    program=$?
    if [[ -f ${1}.expect ]]; then
        diff output ${1}.expect
        diffresult=$?
    else
        diffresult=0
    fi
    if [ ${program} -eq $2 ] && [ ${diffresult} -eq 0 ]; then
        printf "\e[1;32m""OK - ""$(echo ${bench} | awk '{print $2}')""\e[0m"
    else
        printf "\e[1;31m!!! FAILED !!!\e[0m"
    fi
        echo ""
}

echo "-------------- simple tests (all should be OK) -------------- "
for test in add assign bitwise div expr 'for' 'function' 'if' literal mul sub 'while'; do
    run_test ${SIMPLE_TESTS}/${test} 0
done

echo "-------------- my tests (all should be OK) -------------- "
for test in `find ${MY_TESTS} -name '*.mvm'`; do
    run_test ${test%.*} 0
done

echo "-------------- failing test (all should be OK) -------------- "
for test in for_range for_var function-return-void if-fun op_bin op_not op_streq op_sub range; do
    run_test ${FAIL}/${test} 100
done
	run_test ${FAIL}/vars 200

echo "-------------- additional tests (all should be OK) -------------- "
for test in casts complex fib_closure function-call 'function' function-cast vars; do
    run_test ${ADDITIONAL}/${test} 0
done

echo "-------------- optional tests (all should be OK) -------------- "
for test in function_native; do
   run_test ${OPTIONAL}/${test} 0
done

echo "-------------- perf test (please wait for each test about 3 sec) (all should be OK) -------------- "
for test in prime; do
#    for i in {1..20}; do
        run_test ${PERF}/${test} 0
#    done
done

echo "-------------- long tests (please wait for each test about 5-10sec) (all should be OK) -------------- "
for test in ackermann ackermann_closure complex2 fib; do
   run_test ${ADDITIONAL}/${test} 0
done

rm output
