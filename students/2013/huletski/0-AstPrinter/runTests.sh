TEST_SCRIPT="./../../../../tests/astprint/run.py"
TESTEE="./build/debug/mvm"

$TEST_SCRIPT -e $TESTEE -d ./../../../../tests/
$TEST_SCRIPT -e $TESTEE -d ./../../../../tests/optional
$TEST_SCRIPT -e $TESTEE -d ./../../../../tests/additional