#!/bin/bash

set -e

run(){
	NAME=$1
	mkdir -p res/$NAME
	rm -rf 2/$NAME.cachedb
	./igen4 -J2 -crwx -GF 2/$NAME --rep 11 -O res/$NAME/a_{i}.txt -j10 --rep-para 4
	./igen4 -J2 -crwx -GF 2/$NAME          -O res/$NAME/full.txt --full -j16
}

run id
run uname
run cat
run mv
run ln
run date
run join
