import os.path
from time import time
from collections import OrderedDict
import vu_common as CM
import sys

print('Load data from: {}'.format(sys.argv[1]))
assert os.path.isfile(sys.argv[1])
pathconds_d = CM.vload(sys.argv[1])

for pathcond,(covs,samples) in pathconds_d.iteritems():
    for loc in sorted(covs):
        if loc.find("\0") != -1:
            print(repr(loc))
