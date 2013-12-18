TEST_SCRIPT="./../../../../tests/run.py"
TESTEE="./build/opt/mvm"

$TEST_SCRIPT -e $TESTEE -t ./../../../../tests/
$TEST_SCRIPT -e $TESTEE -t ./../../../../tests/optional
# ignore vars.mvm result
$TEST_SCRIPT -e $TESTEE -t ./../../../../tests/additional
