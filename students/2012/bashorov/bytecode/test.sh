#!/bin/bash

GLOBAL_TESTS_ROOT=../../../../tests/
GLOBAL_TESTS_EXPECT=expect_gl/

TEST_BIN=$1
MINE_TESTS_ROOT=$2
MINE_TESTS_EXPECT=$3
SUFF=$4
COLOR=$5
SHOWALL=$6

TEST_OUT=testout

function run {
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
		[ "$RESULT" == "0" ] && [ "$SHOWALL" != "showall" ] && return 0

		if [ "$COLOR" == "coloroff" ]; then
			if [ $RESULT -ne "0" ]; then
				RESULT=" /="
			else
				RESULT=" =="
			fi
		else
			RESULT=""
		fi
	fi

	echo " [$IN -> $OUT]"
	echo " $RESULT"
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
		OUT="$TEST_OUT/$BASEFILENAME.out"
		EXPECT="$TEST_EXPECT$BASEFILENAME.$SUFF.expect"

		# if [ -f $EXPECT ] ; then
			run $i $OUT $EXPECT
		# fi

	done
}

# rm -rf $TEST_OUT
mkdir -p $TEST_OUT

test $MINE_TESTS_ROOT $MINE_TESTS_EXPECT
# test $GLOBAL_TESTS_ROOT $GLOBAL_TESTS_EXPECT