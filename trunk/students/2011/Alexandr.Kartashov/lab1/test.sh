#!/bin/sh

for f in $(ls ../../../../tests/*mvm); do echo "Running test using " $f": "; build/bin/lab1 $f; echo; done