#!/bin/bash

set -e

OUT_PREFIX=res/Analyze_progress

run(){
	NAME=$1
	FLAGS=$2
	rm -rf $OUT_PREFIX/$NAME
	mkdir -p $OUT_PREFIX/$NAME
	./igen4 -J2 -cr $FLAGS -F 2/$NAME -O $OUT_PREFIX/$NAME/iter_{iter}.txt --save-each-iter
}

run id   
run uname -s4
run cat  
run mv   
run ln   
run date 
run join 
run grin 
run ngircd

#run sort 