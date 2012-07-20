#!/usr/bin/python

import os
import re
import sys
import urllib

params = {
    'mapfile': os.path.splitext(os.path.basename(sys.argv[1]))[0],
    'route': sys.argv[2],
    }

u = urllib.urlopen('http://www.undecidable.org.uk/~edwin/cgi-bin/weblifter.cgi', urllib.urlencode(params))
response = u.read()
u.close()

m = re.search(r'Score: ([0-9-]*)', response)
print m.group(1)
