#!/usr/bin/python

import random
import sys

n = int(sys.argv[1])

print ''.join([random.choice('LLLDDDUUURRRWWWZZZZZA') for i in xrange(n)])
