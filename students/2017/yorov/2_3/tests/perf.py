#!/usr/bin/python2.7

import string
import subprocess
import sys
import os

test_root = '/home/sobir/spbau/secondyear/vm/mathvm/tests'
user_root = '/home/sobir/spbau/secondyear/vm/mathvm/students/2017'

vm_dirs = [ 'yorov/2_3']
perf_tests = [ 'perf/newton.mvm', 'perf/graph_plot.mvm',
               'perf/prime.mvm', 'perf/plot.mvm' ]

def runProg(bin, arg, stdin = None):
    pipe = subprocess.Popen([bin, arg],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    (out, err) = pipe.communicate()
    if err:
        print 'ERROR: %s' % (err)
    return out

def runVm(vm_dir):
    user = string.split(vm_dir, '/')[0]
    vm = os.path.join(user_root, vm_dir, 'build/debug/mvm')
    print 'User is %s vm is %s' %(user, vm)
    for t in perf_tests:
        tp = os.path.join(test_root, t)
        print runProg(vm, tp)

def main(argv):
    for vm_dir in vm_dirs:
        runVm(vm_dir)

if __name__ == '__main__':
    main(sys.argv)
