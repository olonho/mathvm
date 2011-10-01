#!/bin/bash
make

VM=./build/bin/gen_code
SRCS=../../../../tests

for src in $SRCS/*.mvm 
do
echo
BASENAME=${src##*/}
FNAME=${BASENAME%.*}
#echo ${FNAME}

echo "======= Working with ${src}: ================"
cat $src
echo
echo "======= Transforming ${src} to stdout ======="
$VM ${src} 
echo
done
