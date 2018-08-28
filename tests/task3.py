#!/usr/bin/env python2.7

import sys
from common import *

def run_test(mvm, test_dir, test):
    try:
        print('Starting test "'+test+'"')
        sys.stdout.flush()

        test_file = os.path.join(test_dir, test + '.mvm')
        expect_file = os.path.join(test_dir, test + '.expect')

        result_data = run_mvm(mvm, '-i', test_file)
        expect_data = read_file(expect_file)

        if expect_data == result_data:
            print('Test "'+test+'" has PASSED')
        else:
            print('Test "'+test+'" has FAILED')
            print('Expected: ')
            print('**************************')
            print(expect_data)
            print('**************************')
            print('Result: ')
            print('**************************')
            print(result_data)
            print('**************************')
            print(make_diff(expect_file, result_data))
            print('**************************')
    except Exception as e:
        print('Failed to execute the test ' + test)
        print(e)

if __name__ == '__main__':
    for mvm, test_dir, test in load_tests():
        run_test(mvm, test_dir, test)
