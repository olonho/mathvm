#!/bin/bash
python ../../../tests/run.py -e build/opt/mvm -t ../../../tests
python ../../../tests/run.py -e build/opt/mvm -t ../../../tests/additional
# python ../../../tests/run.py -e build/opt/mvm -t ../../../tests/optional
# python ../../../tests/run.py -e build/opt/mvm -t ../../../tests/perf
