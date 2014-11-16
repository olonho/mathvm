#!/bin/bash

MATH_VM_TESTS=../../../../tests

SIMPLE_TESTS=${MATH_VM_TESTS}/.
OPTIONAL=${MATH_VM_TESTS}/optional
ADDITIONAL=${MATH_VM_TESTS}/additional
FAIL=${MATH_VM_TESTS}/additional/fail

function run_test {
  echo -n "test:" ${1##*/} "-- "
  ./build/Debug/mvm $1.mvm > output
  program=$?
  if [[ -f $1.expect ]]; then
    diff output $1.expect
    diffresult=$?
  else
    diffresult=0
  fi
  if [ ${program} -eq $2 ] && [ ${diffresult} -eq 0 ]; then
    echo -n "OK"
  fi
  echo ""
}

echo "-------------- simple tests (all should be OK) -------------- "
for test in add assign bitwise div expr 'for' 'function' 'if' literal mul sub 'while'; do
    run_test ${SIMPLE_TESTS}/${test} 0
done

# TODO
#echo "-------------- optional tests -------------- "
#for test in function_native; do
#    run_test ${OPTIONAL}/${test}
#done

# TODO
#echo "-------------- additional tests -------------- "
#for test in casts; do
#    run_test ${ADDITIONAL}/${test}
#done

# TODO
# echo "-------------- failing test (all should be OK) -------------- "
#for test in for_range for_var function-cast function-return-void if-fun op_bin op_not op_streq op_sub range; do
#    run_test ${FAIL}/${test} 100
#done

rm output
