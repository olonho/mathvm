#!/usr/bin/env python2.7

import os
from common import run_mvm


users_root = '../students/2018'
users = []

perf_tests = [
    'perf/newton.mvm',
    'perf/graph_plot.mvm',
    'perf/prime.mvm',
    'perf/plot.mvm',
    'perf/randomart.mvm',
    'perf/lissajous.mvm',
]


def run_perf_tests(user):
    mvm = os.path.join(users_root, user, 'build/opt/mvm')
    print 'User: ', user
    for test in perf_tests:
        print run_mvm(mvm, '-j', test)


if __name__ == '__main__':
    for user in users:
        run_perf_tests(user)
