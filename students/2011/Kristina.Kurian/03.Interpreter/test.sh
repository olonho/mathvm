MVM=./build/debug/interpreter
TEST_FILES=../../../../tests
FILES="var literal add sub mul div priority expr for if while function"
TEST_FILES_ADD=../../../../tests/additional
#FILES_ADD="3function casts closure function-call function-cast function vars"
FILES_ADD="function-call function-cast function 3function cast closure"

echo -e "\e[1;31m**************Testing is started!*************\e[0m"

echo "**************Default*************************"
echo "======= Original ======="
echo "double x; double y;"
echo "x += 8.0; y = 2.0;" 
echo "print('Hello, x=',x,' y=',y,'\n');"
echo "======= Result ========="
$MVM

for t in $FILES 
do
echo
echo -e "\e[1;34m**************Testing file "$t.mvm"***************\e[0m"
echo "======= Original ======="
cat $TEST_FILES/$t.mvm
echo "======= Prediction ====="
cat $TEST_FILES/$t.expect
echo "======= Result ========="
$MVM $TEST_FILES/$t.mvm
done

echo
echo -e "\e[1;31m**************Testing is finished! ************\e[0m"
