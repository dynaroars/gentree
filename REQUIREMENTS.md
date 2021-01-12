### Hardware
- The benchmarks presented in the paper was run on a workstation with an 64-core AMD Ryzen Threadripper 3990X @ 2.9 GHz CPU, 64 GB RAM, and at least 40GB of free disk space.
    + Running all benchmarks takes around 26 hours.
- Some benchmarks could be run in less than 10 minutes on a normal 4-core laptop (the motivation example and all coreutils programs, except `sort` and `ls`).

### Software
- Linux-based OS (tested on Ubunutu 20.04 and Debian 10.7).
- Docker (tested with Docker 19.03.14 and 20.10.1).
    + Make sure you can run `docker run hello-world` successfully on the host machine.
