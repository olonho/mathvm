#!/bin/bash
make

VM=./build/opt/main
SRCS=../../../../tests/

for src in $SRCS/*.mvm 
do
echo
BASENAME=${src##*/}
FNAME=${BASENAME%.*}
#echo ${FNAME}

echo "======= Running ${src}: ================"
echo "Expected result:"
cat ${SRCS}/${FNAME}.expect
echo
echo "Actual result:"
$VM ${src} 
echo
done
