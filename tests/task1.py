#!/usr/bin/env python2.7

from common import *


def run_test(mvm, test_dir, test):
    try:
        test_file = os.path.join(test_dir, test + '.mvm')
        temp_file = os.path.join('/tmp', 'temp.mvm')
        # temp_file = os.path.join('/tmp', test + '.mvm')
        test_data = read_file(test_file)

        input1_file = test_file
        result1_data = run_mvm(mvm, '-p', input1_file)

        input2_file = temp_file
        write_file(input2_file, result1_data)
        result2_data = run_mvm(mvm, '-p', input2_file)

        eq_pass = result1_data == result2_data
        kw_pass = kw_compare(test_data, result1_data)

        if eq_pass and kw_pass:
            print 'Test "' + test + '" has PASSED'
        else:
            print 'Test "' + test + '" has FAILED'
            if not eq_pass:
                print_result(input2_file, result2_data, False)
            elif not kw_pass:
                print 'Output doesn\'t look like input (by our checker). Have you missed something?'
                print_result(test_file, result1_data, False)
    except Exception as e:
        print 'Failed to execute the test ' + test + ': ', e.message


if __name__ == '__main__':
    for mvm, test_dir, test in load_tests():
        run_test(mvm, test_dir, test)
