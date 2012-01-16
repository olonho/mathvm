MVM=./build/debug/translator
TEST_FILES=../../../../tests

echo "**************Testing is started!*************"

echo "**************Default*************************"
echo "======= Original ======="
echo "double x; double y;"
echo "x += 8.0; y = 2.0;" 
echo "print('Hello, x=',x,' y=',y,'\n');"
echo "===== Translation ======"
$MVM

for t in $TEST_FILES/*.mvm 
do
echo
echo "**************Testing file "$t"***************"
echo "======= Original ======="
cat $t
echo "===== Translation ======"
$MVM $t
done

echo
echo "**************Testing is finished! ************"
