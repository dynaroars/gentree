#!/bin/bash

set -e

IN_PREFIX=res/Analyze_progress
OUT_PREFIX=res/Analyze_progress_rand

run(){
	NAME=$1
	FLAGS=$2
	X=$(cat $IN_PREFIX/$NAME.csv | tail +2 | head -n -4 | cut -d , -f 2 | paste -sd "," -)
	rm -rf $OUT_PREFIX/$NAME
	mkdir -p $OUT_PREFIX/$NAME
	./igen4 -J2 -cr $FLAGS -F 2/$NAME -O $OUT_PREFIX/$NAME/iter_{iter}.txt --rand-each-iteration $X
}

run id
run uname 
run cat
run mv
run ln
run date
run join
run grin
run ngircd

# run sort