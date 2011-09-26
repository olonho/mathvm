#!/bin/bash
make
mkdir ./testres
rm ./testres/*.mvm

VM=./build/bin/show_src
SRCS=../../../../tests

for src in $SRCS/*.mvm 
do
echo
BASENAME=${src##*/}
FNAME=${BASENAME%.*}
#echo ${FNAME}

echo "======= Working with ${src}: ======="
cat $src
echo "======= Transforming ${src} to ./testres/${FNAME}.mvm ======="
$VM $src > ./testres/${FNAME}.mvm
echo
echo "======= Transforming ./testres/${FNAME}.mvm to stdout ======="
$VM ./testres/${FNAME}.mvm
echo
done
