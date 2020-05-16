#!/bin/bash

set -e

run(){
	NAME=$1
	N_RAND=$2
	OUT_PREFIX=res/Analyze_rand/$NAME/
	mkdir -p $OUT_PREFIX

	./igen4 -J2 -cr -GF 2/$NAME -O $OUT_PREFIX/a_{i}.txt --rep 11 --rep-para 11 \
				--rand $N_RAND -R0
}

run id 622
run uname 784
run cat 2304
run mv 4764
run ln 2995
run date 14359
run join 4639
run sort 186801

run grin 7644
run ngircd 45509
