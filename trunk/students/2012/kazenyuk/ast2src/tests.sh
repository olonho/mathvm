#!/bin/sh

TESTS_ROOT=../../../../tests/
TEST_BIN=build/debug/ast2src
TEST_OUT=/tmp/ks/spbau/mathvm/ast2src/out

mkdir -p $TEST_OUT
for i in $(find $TESTS_ROOT -type f -iname "*.mvm"); do
	BASEFILENAME=$(basename $i)

	$TEST_BIN $i > $TEST_OUT/$BASEFILENAME".out"
	RESULT=$?

	echo -n "$i P1 "
	if [ $RESULT -ne "0" ]; then
		echo "NOK"
	else
		echo "OK"
	fi

	$TEST_BIN $TEST_OUT/$BASEFILENAME".out" > $TEST_OUT/$BASEFILENAME".out2"
	RESULT2=$?

	echo -n "$i P2 "
	if [ $RESULT2 -ne "0" ]; then
		echo "NOK"
	else
		echo "OK"
	fi

	echo
done
