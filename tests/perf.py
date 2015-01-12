#!/usr/bin/python

import string
import subprocess
import sys
import os

test_root = '/home/nike/mathvm/mathvm/tests'
user_root = '/home/nike/mathvm/mathvm/students/2014'

vm_dirs = [ 'afanasjev/2', 'amanov/2', 'atamas/3', 
            'bugaev/2', 'habibulin/2_1', 'kalakuzkij/3',
            'kartashov/3', 'kovalenko/2+3', 'kryschenko/2-3',
            'novokreschenov/2', 'obedin/3', 'ordijan/2',
            'turaev.m/2', 'turaev.t/2', 'ustuzhanina/3',
            'voronchihin/3', 
            'zharkov/2',
            'zhirkov/vm', 'zvetkov/2']
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
    vm = os.path.join(user_root, vm_dir, 'build/opt/mvm')
    print 'User is %s vm is %s' %(user, vm)
    for t in perf_tests:
        tp = os.path.join(test_root, t)
        print runProg(vm, tp)

def main(argv):
    for vm_dir in vm_dirs:
        runVm(vm_dir)

if __name__ == '__main__':
    main(sys.argv)
