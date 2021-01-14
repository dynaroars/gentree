# Quick Start
The development and experiment environment is provided as a single Docker image at `unsatx/gentree_docker:icse21`.

**1. Prerequisites:**
- Linux-based OS (tested on Ubunutu 20.04 and Debian 10.7).
- Docker (tested with Docker 19.03.14 and 20.10.1).
  - Make sure you can run `docker run hello-world` successfully on the host machine.

**2. Pull Docker image:**
```bash
docker pull unsatx/gentree_docker:icse21
```

**3. Run container:**
```bash
docker run -it --rm --tmpfs /mnt/ramdisk unsatx/gentree_docker:icse21 bash
```
Alternatively, you could use fish shell inside container:
```bash
docker run -it --rm --tmpfs /mnt/ramdisk unsatx/gentree_docker:icse21 fish
```

**4. Test GenTree (inside container):**
```bash
cd ~/gentree/wd

./gt -J2 -cx -BF @ex_paper        # figure 1b
./gt -J2 -cx -BF @ex_paper --full # figure 1b (all configurations)

./gt -J2 -cx -GF 2/id             # coreutils `id` (C)
./gt -J2 -cx -YF 2/vsftpd         # vsftpd (Otter)
```

**Note:** if you get permission error on directory `/mnt/ramdisk` while running GenTree, run the following command inside the container: `sudo chmod 777 -R /mnt/ramdisk`

# Output
If GenTree is working correctly, for each interaction, it outputs:
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
(...)
```
