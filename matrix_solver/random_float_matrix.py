#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import random

try:
    dim_x, dim_y = int(sys.argv[1]), int(sys.argv[2])
except Exception, e:
    sys.stderr.write("WARNING!, Error parsing matrix dimensions ...\n")
    raise


for row in xrange(0, dim_x):
    print "\t".join([ unicode(random.uniform(0, 9999)) for x in xrange(0, dim_y) ])
