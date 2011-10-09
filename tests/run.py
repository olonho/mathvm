#!/usr/bin/python

import subprocess
import sys
import os
import getopt

def runProg(bin, arg):
    pipe = subprocess.Popen([bin, arg],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    (out, err) = pipe.communicate()
    return out

def readFile(name):
    f = open(name, 'r')
    return f.read()

diff = '/usr/bin/diff'
tests = ['literal', 'add', 'sub', 'mul', 'div', 'expr', 'for', 'while', 'if', 'function'];

def runTest(mvm, root, test):
    result = runProg(mvm, os.path.join(root, test+'.mvm'))
    expectFile = os.path.join(root, test+'.expect')
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
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'a:k:', ['add=', 'kind='])
        add_dir = None
        kind = 'debug'
        for o, a in opts:
            if o == '-a':
              add_dir = a
            if o =='-k':
              kind = a
        mvm = os.path.join('./build', kind, 'mvm')
        if add_dir is not None:
          import glob
          import re
          add_tests = glob.glob(os.path.join(add_dir, '*.mvm'))
          for tp in add_tests:
              m = re.search(r"([\w]+)\.mvm", tp)
              if m is None:
                  continue
              t = m.group(1)
              runTest(mvm, add_dir, t)
        for t in tests:
            runTest(mvm, './tests', t)
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err)
        print "A"
        sys.exit(2)

if __name__ == '__main__':
    main(sys.argv)
