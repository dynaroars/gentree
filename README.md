# Abstract

Modern software systems are increasingly designed to be highly configurable, which increases flexibility but can make programs harder to develop, test, and analyze, e.g., how configuration options are set to reach certain locations, what characterizes the configuration space of an interesting or buggy program behavior?
We introduce *GenTree*, a new dynamic analysis that automatically learns a program's *interactions* - logical formulae that describe how configuration option settings map to code coverage.
*GenTree* uses an iterative refinement approach that runs the program under a small sample of configurations to obtain coverage data; uses a custom classifying algorithm on these data to build decision trees representing interaction candidates; and then analyzes the trees to generate new configurations to further refine the trees and interactions in the next iteration.
Our experiments on 17 configurable systems spanning 4 languages show that *GenTree* efficiently finds precise interactions using a tiny fraction of the configuration space.

# GenTree

This repository contains an implementation of the GenTree algorithm, as described in the paper:
> KimHao Nguyen and ThanhVu Nguyen. 2021. GenTree: Using Decision Trees to Learn Interactions for Configurable Software. In International Conference on Software Engineering.

The full paper could be found at: [paper.pdf](https://github.com/unsat/gentree/releases/download/submit_icse21/gentree-icse21.pdf)

# Quick Start
The development and experiment environment is provided as a single Docker image at `unsatx/gentree_docker:icse21`.

**1. Prerequisites:**
- Linux-based OS (tested on Ubunutu 20.04 and Debian 10.7)
- Docker (tested with Docker 19.03.14 and 20.10.1)

**2. Run container:**
```bash
docker run -it --rm --tmpfs /mnt/ramdisk unsatx/gentree_docker:icse21 bash
```
Alternatively, you could use fish shell inside container:
```bash
docker run -it --rm --tmpfs /mnt/ramdisk unsatx/gentree_docker:icse21 fish
```

**3. Run GenTree (inside container):**
```bash
cd ~/gentree/wd
./gt -J2 -cx -BF @ex_paper        # figure 1b
./gt -J2 -cx -BF @ex_paper --full # figure 1b (all configurations)
./gt -J2 -cx -GF 2/id             # coreutils `id` (C)
./gt -J2 -cx -GF 2/grin -j 8      # grin (Python), using 8 cores
./gt -J2 -cx -YF 2/vsftpd         # vsftpd (Otter)
./gt --help                       # help
```

**Note 1:** if you get permission error on directory `/mnt/ramdisk` while running GenTree, run the following command inside the container: `sudo chmod 777 -R /mnt/ramdisk`

**Note 2:** for the list of all runnable tests, refer to section *Test Programs* below.

### Download Docker image manually
If, for some reasons, `unsatx/gentree_docker:icse21` is not available in the Docker Hub, you can downoad and import it from Github releases.
```bash
wget https://github.com/unsat/gentree/releases/download/submit_icse21/gentree_docker-icse21.tgz.a
wget https://github.com/unsat/gentree/releases/download/submit_icse21/gentree_docker-icse21.tgz.b
cat gentree_docker-icse21.tgz.a gentree_docker-icse21.tgz.b | gunzip -c - | docker load
```


# Output
For each interaction, GenTree outputs:
- M/H: number of hits/miss configs
- Last rebuild: when the final decision tree was built
- Locations
- Interaction formula in SMT-LIB v2 format
- Pre-order traversal of the decision tree

```
$ ./gt -J2 -cx -BF @ex_paper
(...)
# seed = 123, repeat_id = 0, thread_id = 0
#      171    9    9 |     0    -1 |  0    6 |     0     0      0 | 5bfa86673df79d5874cfa166ffc74067
======
# M/H: 0 / 186
# Last rebuild:   iter 1  num_configs 3
L0, 
-
true
-
H
======
# M/H: 87 / 98
# Last rebuild:   iter 2  num_configs 27
L1, 
-
(or (= a |1|) (= b |2|))
-
5 6 MMHH 6 MMH
======
# M/H: 51 / 137
# Last rebuild:   iter 2  num_configs 30
L4, 
-
(or (= u |0|) (= v |0|))
-
3 H 4 HM
======
# M/H: 79 / 110
# Last rebuild:   iter 3  num_configs 50
L6, 
-
(let ((a!1 (or (= e |0|) (= e |1|) (= s |0|))))
  (or (and (= v |0|) a!1) (and (= u |0|) a!1)))
-
4 1 H 0 HHM 3 1 H 0 HHMM
======
# M/H: 145 / 52
# Last rebuild:   iter 1  num_configs 15
L3, 
-
(and (= v |1|) (= u |1|))
-
4 M 3 MH
======
# M/H: 144 / 42
# Last rebuild:   iter 3  num_configs 47
L7, 
-
(and (= s |0|) (= e |2|) (or (= u |0|) (= v |0|)))
-
1 0 MM 4 H 3 HMM
======
# M/H: 182 / 38
# Last rebuild:   iter 5  num_configs 102
L5, 
-
(and (= s |1|) (= e |2|) (or (= u |0|) (= v |0|)))
-
1 M 0 MM 3 H 4 HM
======
# M/H: 197 / 22
# Last rebuild:   iter 6  num_configs 162
L2, 
-
(let ((a!1 (or (and (= b |0|) (or (= a |0|) (= a |2|)))
               (and (= b |1|) (or (= a |0|) (= a |2|))))))
  (and (= d |1|) (= c |0|) a!1))
-
8 M 7 6 5 HMH 5 HMHMMMM
======
# M/H: 163 / 28
# Last rebuild:   iter 4  num_configs 58
L8, 
-
(and (= s |0|)
     (= e |2|)
     (or (and (= u |0|) (= v |1|)) (and (= u |1|) (= v |0|))))
-
1 0 MM 3 4 MH 4 HMM
======
```

# Analysis
- **Prepare data (inside container):**
```bash
cd ~/gentree/wd
mkdir rcat
# Run with all configurations to obtain ground truth
./gt -J2 -cx -GF 2/cat -O "rcat/full.txt" --full -j 8
# Run 11 times, output to rcat/ directory
./gt -J2 -cx -GF 2/cat -O "rcat/run_{i}.txt" --rep 11 --rep-parallel 8 -j 8
```

- **Compare result with ground truth:**
```bash
./gt -A0 -T0 -GF 2/cat -I rcat/full.txt,rcat/run_0.txt
# Counting total cex
# R[ 0] ================================  FINAL RESULT  ================================
# R[ 0] Total: diff  0  , miss  0  , locs A 205 , B 205 
# R[ 0] Uniq : diff  0  , miss  0  , locs A  27 , B  27 , cex 0
# R[ 0] Total CEX: 0
```

- **Compare result of 11 runs with ground truth:**
```bash
./gt -A0 -T3 -GF 2/cat -I rcat/full.txt,rcat/run_{i}.txt --rep 11 -P rcat/stat.csv
cat rcat/stat.csv
```
| _repeat_id | _thread_id | avg_f    | avg_mcc  | cnt_exact | cnt_interactions | cnt_wrong | delta_locs | n_configs | wrong_locs |
| ---------- | ---------- | -------- | -------- | --------- | ---------------- | --------- | ---------- | --------- | ---------- |
| 0          | 0          | 1.000000 | 1.000000 | 27        | 27               | 0         | 0          | 1514      |            |
| 1          | 0          | 1.000000 | 1.000000 | 27        | 27               | 0         | 0          | 1481      |            |
| 2          | 0          | 1.000000 | 1.000000 | 27        | 27               | 0         | 0          | 1924      |            |
| 3          | 0          | 1.000000 | 1.000000 | 27        | 27               | 0         | 0          | 1724      |            |
| 4          | 0          | 1.000000 | 1.000000 | 27        | 27               | 0         | 0          | 1664      |            |
| 5          | 0          | 1.000000 | 1.000000 | 27        | 27               | 0         | 0          | 1455      |            |
| 6          | 0          | 1.000000 | 1.000000 | 27        | 27               | 0         | 0          | 1690      |            |
| 7          | 0          | 1.000000 | 1.000000 | 27        | 27               | 0         | 0          | 1572      |            |
| 8          | 0          | 1.000000 | 1.000000 | 27        | 27               | 0         | 0          | 2011      |            |
| 9          | 0          | 0.999982 | 0.999976 | 26        | 27               | 1         | 0          | 1660      | cat.c:197  |
| 10         | 0          | 1.000000 | 1.000000 | 27        | 27               | 0         | 0          | 1506      |            |
|            |            |          |          |           |                  |           |            |           |            |
| MED        | 0          | 1        | 1        | 27        | 27               | 0         | 0          | 1660      | nan        |
| SIR        | 0          | 0        | 0        | 0         | 0                | 0         | 0          | 109       | nan        |
| MEAN       | 0          | 0.999998 | 0.999998 | 26.9091   | 27               | 0.0909091 | 0          | 1654.64   | nan        |
| MIN        | 0          | 0.999982 | 0.999976 | 26        | 27               | 0         | 0          | 1455      | nan        |
| MAX        | 0          | 1        | 1        | 27        | 27               | 1         | 0          | 2011      | nan        |


- **Find minimum covering configurations size (Section 6.3):**
```bash
./gt -A0 -T4 -GF 2/cat -I rcat/run_0.txt
# min_c = 6
```

# Test Programs

| Language  | Name        | Run command                      |
| --------- | ----------- | -------------------------------- |
| C         | id          | `./gt -J2 -cx -GF 2/id`          |
| C         | uname       | `./gt -J2 -cx -GF 2/uname`       |
| C         | cat         | `./gt -J2 -cx -GF 2/cat`         |
| C         | mv          | `./gt -J2 -cx -GF 2/mv`          |
| C         | ln          | `./gt -J2 -cx -GF 2/ln`          |
| C         | date        | `./gt -J2 -cx -GF 2/date`        |
| C         | join        | `./gt -J2 -cx -GF 2/join`        |
| C         | sort        | `./gt -J2 -cx -GF 2/sort`        |
| C         | ls          | `./gt -J2 -cx -GF 2/ls`          |
|           |             |                                  |
| Python    | grin        | `./gt -J2 -cx -GF 2/grin`        |
| Python    | pylint      | `./gt -J2 -cx -GF 2/pylint`      |
| Ocaml     | unison      | `./gt -J2 -cx -GF 2/unison`      |
| Ocaml     | bibtex2html | `./gt -J2 -cx -GF 2/bibtex2html` |
| Perl      | cloc        | `./gt -J2 -cx -GF 2/cloc`        |
| Perl      | ack         | `./gt -J2 -cx -GF 2/ack`         |
|           |             |                                  |
| C (Otter) | vsftpd      | `./gt -J2 -cx -YF 2/vsftpd`      |
| C (Otter) | ngircd      | `./gt -J2 -cx -YF 2/ngircd`      |

**Note:** It is recommended to use multicores runner for faster execution. E.g., add `-j 16` to use upto 16 cores.

# Hacking

GenTree uses C++17, CMake, and the following libraries:
- Boost     (https://www.boost.org/)
- fmt (https://github.com/fmtlib/fmt)
- GLog (https://github.com/google/glog)
- RocksDB (https://github.com/facebook/rocksdb)
- Z3 (https://github.com/Z3Prover/z3)
- Zstd (https://github.com/facebook/zstd)

The Docker image contains all the development tools needed for GenTree, including the source code and debug builds of all libraries.

- **Run Debug build**
```bash
cd ~/gentree/wd
export LD_LIBRARY_PATH=$HOME/cpplib/install/lib/
./gt_debug -J2 -cx -BF @ex_paper
```

- **Modify/rebuild GenTree**
```bash
cd ~/gentree/wd
./build_debug.sh    # Build Debug (./gt_debug)
./build_rdi.sh      # Build RelWithDebInfo (./gt)
```

- **Tweak GenTree runtime parameters**
```bash
nano ~/gentree/wd/iter2_config.js
```

# Copyright
**(TODO)**