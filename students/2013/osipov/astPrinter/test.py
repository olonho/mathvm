#!/usr/bin/python
import subprocess

executable = "./build/debug/main"
script = "./../../../../tests/astprint/run.py"

test1 = script + " -e " + executable + " -d " + "./../../../../tests/"
test2 = script + " -e " + executable + " -d " + "./../../../../tests/additional"
test3 = script + " -e " + executable + " -d " + "./../../../../tests/optional"

subprocess.call(test1, shell=True)
subprocess.call(test2, shell=True)
subprocess.call(test3, shell=True)

