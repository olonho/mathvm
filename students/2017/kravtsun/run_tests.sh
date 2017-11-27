#!/bin/bash
python ../../../tests/run.py -e build/debug/mvm -t ../../../tests
python ../../../tests/run.py -e build/debug/mvm -t ../../../tests/additional
