#!/bin/bash

./test.sh 2> /dev/null | grep PASSED 
echo ""
./test.sh 2> /dev/null | grep FAILED
