#!/usr/bin/env python2.7

import os

from common import run_mvm

user_root = '../students/2018'

vm_dirs = []
perf_tests = [ 'perf/newton.mvm', 'perf/graph_plot.mvm',
               'perf/prime.mvm', 'perf/plot.mvm' ]

def run_perf_tests(vm_dir):
    user = vm_dir.split('/')[0]
    vm = os.path.join(user_root, vm_dir, 'build/opt/mvm')
    print('User is', user, 'vm is', vm)
    for test in perf_tests:
        print(run_mvm(vm, '-j', test))

if __name__ == '__main__':
    for vm_dir in vm_dirs:
        run_perf_tests(vm_dir)
