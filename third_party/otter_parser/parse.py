import os.path
from time import time
from collections import OrderedDict
import vu_common as CM
import sys
import gzip

#Utilities to work with Otter's data

def read_config_from_line(l):
    """
    Read a config from a string
    >>> print read_config_from_line("b=2")
    b=2
    >>> print read_config_from_line("b=2 a=1")
    b=2 a=1
    """

    return frozenset([tuple(setting.split("=")) for setting in l.strip()])

def format_pc(lines):
    """
    (Re)format path constraint string
    Input: a list of string
    Output: a string
    """

    rs = []
    parts = []
    for l in lines:

        if l.endswith(","):
            parts.append(l)
        else:
            if parts:
                l = ''.join(parts+[l])
                parts = []

            rs.append(l)
    
    assert rs, rs
    if len(rs) == 1:
        rs = rs[0]
    else:
        rs = 'and({})'.format(','.join(rs))

    return rs

def read_exp_part_pc(lines):
    cur_part = []
    samples = []
    for l in lines:
        if (l.startswith('Sample value') or 
            l.startswith('The lines hit were')):
            if cur_part:
                if cur_part[0].startswith('Path condition'):
                    pathcond = format_pc(cur_part[1:])
                    cur_part = []
                elif cur_part[0].startswith('Sample value'):
                    sample = cur_part[1:]
                    if sample:
                        config = [tuple(setting.split("="))
                                  for setting in sample]
                        samples.append(frozenset(config))
                    cur_part = []
     
        cur_part.append(l)

    covs = frozenset(cur_part[1:]) #skip "The lines hit were" line
    pc = (pathcond,covs,samples)
    return pc
    
def read_exp_pc(lines):
    parts = []
    cur_part = []
    for l in lines:
        if l.startswith('Test case'):
            continue

        if l.startswith("--------------------"):
            if cur_part:
                parts.append(cur_part)
                cur_part = []
        else:
            cur_part.append(l)       

    pathconds = [read_exp_part_pc(p) for p in parts]
    return pathconds

def read_exp_dir_pc(exp_dir):
    exp_files = [f for f in os.listdir(exp_dir)
                 if f.endswith('.exp') and "__v1." not in f]
    exp_files = sorted(exp_files)
    exp_files = [os.path.join(exp_dir,f) for f in exp_files]

    print("parse {} exp files from '{}'".format(len(exp_files),exp_dir))
    db = OrderedDict()
    for i,exp_file in enumerate(exp_files):
        print("{}/{}. '{}'".format(i+1,len(exp_files),exp_file))
        pathconds = read_exp_pc(CM.iread_strip(exp_file))
        db[exp_file] = pathconds
    return db
    

def combine_pathconds(db):
    pathconds_d = {}
    for filename,pathconds in db.iteritems():
        for (pathcond,covs,samples) in pathconds:
            if pathcond in pathconds_d:
                covs_,samples_ = pathconds_d[pathcond]
                for sid in covs:
                    covs_.add(sid)
                for sample in samples:
                    samples_.add(sample)
            else:
                pathconds_d[pathcond] = (set(covs),set(samples))

    return pathconds_d


def get_dom(dom_file):
    assert os.path.isfile(dom_file), dom_file

    def get_lines(lines):
        rs = (line.split() for line in lines)
        rs_ = {}
        var_idx = 0
        for parts in rs:
            var = parts[0]
            vals = { "__id": var_idx }
            var_idx = var_idx + 1
            idx = 0
            for lbl in parts[1:]:
                vals[lbl] = idx;
                idx = idx + 1;
            rs_[var] = vals
        return rs_

    return get_lines(CM.iread_strip(dom_file))


def process_otter(exp_dir, dom_file, out_file):
    """
    dom_file = '../alg/exp_dir/1/dom'
    exp_dir = '../alg/exp_dir/1/'

    dom_file = '/Users/tnguyen/Src/Devel/iTree_stuff/expData/vsftpd/possibleValues.txt'
    exp_dir = '/Users/tnguyen/Src/Devel/iTree_stuff/expData/vsftpd/rawExecutionPaths'
    
    pathconds: 8676, samples 4365972 (real 16040), covs 10927220 (real 7741) (11.7382318974s)

    samples: 13796
    covs: 2549

    """
    
    st = time()
    db = read_exp_dir_pc(exp_dir)
    print("read {} exp files ({}s)".format(len(db),time()-st))

    st = time()
    pathconds_d = combine_pathconds(db)
    allcovs,allsamples = set(),set()
    ncovs,nsamples = 0,0
    covd = {}

    for covs,samples in pathconds_d.itervalues():
        for sid in covs:
            allcovs.add(sid)
            covd[sid] = 0
        ncovs += len(covs)

        for sample in samples:
            allsamples.add(sample)
        nsamples += len(samples)        
    
    print("pathconds: {}, covs {} (real {}), samples {} (real {}) ({}s)"
          .format(len(pathconds_d),
                  ncovs,len(allcovs),
                  nsamples,len(allsamples),
                  time() - st))
    

    cov_id = 0
    cov_sorted = sorted(covd.keys())
    for k in cov_sorted:
        covd[k] = cov_id
        cov_id = cov_id + 1


    dom = get_dom(dom_file)
    #print("DOM: {}", dom)

    #f = open(out_file, "wb")
    f = gzip.open(out_file, 'wb', 9)
    f.write('{} {}\n'.format(len(dom), len(cov_sorted)))
    for c in cov_sorted:
        f.write(c)
        f.write(' ')
    f.write('\n############\n')

    #f.close()
    #return

    for pathcond,(covs,samples) in pathconds_d.iteritems():
        for loc in sorted(covs):
            #f.write(loc + ', ')
            assert loc in covd
            f.write(str(covd[loc]))
            f.write(' ')
        f.write('\n------\n')
        f.write(pathcond)
        f.write('\n------\n')

        a = [-1] * len(dom)
        for c in samples:
            for var,val in c:
                assert var in dom
                M = dom[var];
                assert val in M
                a[M['__id']] = M[val]
            f.write(','.join(str(item) for item in a))
            f.write('\n')

        f.write('\n============\n')
    f.close()


    return pathconds_d


process_otter(sys.argv[1], sys.argv[2], sys.argv[3])


