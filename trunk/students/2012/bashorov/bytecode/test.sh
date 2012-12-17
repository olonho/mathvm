#!/bin/bash

# TEST_BIN=""
# TESTS_ROOT="./tests"
# TESTS_EXPECT="./tests"
# SUFF="test"

TEST_BIN=$1
TESTS_ROOT=$2
TESTS_EXPECT=$3
SUFF=$4
COLOR=$5
SHOWALL=$6

TEST_OUT=testout
RUNNED_COUNT=0
PASSED_COUNT=0

function run {
	RUNNED_COUNT=$[$RUNNED_COUNT+1]
	IN=$1
	OUT=$2
	EXPECT=$3

	$TEST_BIN $IN > $OUT
	RESULT=$?

	if [ $RESULT -ne "0" ]; then
		colorErr
		RESULT="ERR = $RESULT"
	else
		check $OUT $EXPECT
		RESULT=$?

		if [ "$COLOR" == "coloroff" ]; then
			if [ $RESULT -ne "0" ]; then
				RESULT_TEXT=" /="
			else
				RESULT_TEXT=" =="
				PASSED_COUNT=$[$PASSED_COUNT+1]
			fi
		else
			RESULT_TEXT=""
		fi

		[ "$RESULT" == "0" ] && [ "$SHOWALL" != "showall" ] && (PASSED_COUNT=$[$PASSED_COUNT+1]) && return 0
	fi

	echo " [$IN -> $OUT]"
	echo " $RESULT_TEXT"
	colorOff
}

function colorErr {
	if [ "$COLOR" != "coloroff" ]; then
		echo -en '\E[;31m'"\033[1m"
	fi	
}

function colorOk {
	if [ "$COLOR" != "coloroff" ]; then
		echo -en '\E[;32m'"\033[1m" 
	fi	
}

function colorOff {
	if [ "$COLOR" != "coloroff" ]; then
		echo -en "\033[0m"
	fi
}

function check {
	OUT=$1
	EXPECT=$2

	if diff $OUT $EXPECT >/dev/null ; then
		colorOk
		if [ "$COLOR" == "coloroff" ]; then
			return 0
		fi
	else
		colorErr
		if [ "$COLOR" == "coloroff" ]; then
			return 1
		fi
	fi
}

function test {
	TESTS_ROOT=$1
	TEST_EXPECT=$2

	for i in $(find $TESTS_ROOT -type f -iname "*.mvm"); do
		BASEFILENAME=$(basename $i ".mvm")
		OUT="$TEST_OUT/$BASEFILENAME.$SUFF.out"
		EXPECT="$TEST_EXPECT$BASEFILENAME.$SUFF.expect"

		# if [ -f $EXPECT ] ; then
			run $i $OUT $EXPECT
		# fi

	done

	echo
	if [ "$RUNNED_COUNT" == "$PASSED_COUNT" ]; then
		echo "All passed ($PASSED_COUNT)"
	else
		echo "Passed: $PASSED_COUNT"
		echo "Runned: $RUNNED_COUNT"
	fi
}

# rm -rf $TEST_OUT
mkdir -p $TEST_OUT

test $TESTS_ROOT $TESTS_EXPECT