#!/bin/bash

set -e

run(){
	NAME=$1
	mkdir -p res/$NAME
	rm -rf 2/$NAME.cachedb
	./igen4 -J2 -crwx -YF 2/$NAME --rep 11 -O res/$NAME/a_{i}.txt -j10 --rep-para 4
	if [ "$2" = "full" ]; then
		./igen4 -J2 -crwx -YF 2/$NAME -O res/$NAME/full.txt --full -j16
	fi
}

run vsftpd notfull
run ngircd full