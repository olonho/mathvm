#!/usr/bin/python

import subprocess
import sys

def runProg(bin, arg):
    pipe = subprocess.Popen([bin, arg],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    (out, err) = pipe.communicate()
    return out

def readFile(name):
    f = open(name, 'r')
    return f.read()

mvm = './build/bin/mvm'
diff = '/usr/bin/diff'
tests = ['literal', 'add', 'sub', 'mul', 'div', 'for', 'if'];

def runTest(test):
    result = runProg(mvm, './tests/'+test+'.mvm')
    expectFile = './tests/'+test+'.expect'
    expect = readFile(expectFile)
    if (expect == result):
        print 'Test "'+test+'" has PASSED'
    else:
        print 'Test "'+test+'" has FAILED'
        print 'Expected: '
        print '**************************'
        print expect
        print 'Result: '
        print '**************************'
        print result
        print '**************************'
        pipe = subprocess.Popen([diff, '-u', expectFile, '-'],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        (out, err) = pipe.communicate(result)
        print out
        

def main(argv):
    for t in tests:
        runTest(t)

if __name__ == '__main__':
    main(sys.argv)
