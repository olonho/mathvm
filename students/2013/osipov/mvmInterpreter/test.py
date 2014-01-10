#!/usr/bin/python
import subprocess

executable = "./build/debug/mvm"
script = "./../../../../tests/run.py"

test = script + " -e " + executable 

subprocess.call(test, shell=True)

