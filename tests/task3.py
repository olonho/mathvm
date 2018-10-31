#!/usr/bin/env python2.7

import sys
from common import *


def run_test(mvm, test_dir, test):
    try:
        # print('------------------------------------------------')
        print('Starting test "' + test + '"')
        sys.stdout.flush()

        test_file = os.path.join(test_dir, test + '.mvm')
        expect_file = os.path.join(test_dir, test + '.expect')

        result_data = run_mvm(mvm, '-i', test_file)
        expect_data = read_file(expect_file)

        if expect_data == result_data:
            return True, ''
        else:
            message = 'Expected: \n'
            message += '--------------------------\n'
            message += expect_data + '\n'
            message += '--------------------------\n'
            message += 'Result: \n'
            message += '--------------------------\n'
            message += result_data
    except Exception as e:
        message = e.message
    return False, message


if __name__ == '__main__':
    passed_tests = []
    failed_tests = {}
    for mvm, test_dir, test in load_tests():
        passed, message = run_test(mvm, test_dir, test)
        if passed:
            passed_tests.append(test)
        else:
            failed_tests[test] = message

    print('\n')

    if len(failed_tests) == 0:
        print('All tests passed')
        exit(0)

    if len(passed_tests) > 0:
        print('PASSED:')
        for test in sorted(passed_tests):
            print('Test: ' + test)
    if len(failed_tests) > 0:
        if len(passed_tests) > 0:
            print()
            print('------------------------------------------------')
        print('FAILED')
        for test in sorted(failed_tests.keys()):
            print('---------------------------------')
            print('Test: ' + test)
            print(failed_tests[test])
