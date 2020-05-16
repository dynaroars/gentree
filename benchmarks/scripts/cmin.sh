#!/bin/bash

set -e

PREFIX=res/Analyze/cmin

analyze(){
	NAME=$1
	FNAME=${2:-full.txt}
	./igen4 -A0 -T4 -F 2/$NAME -I res/$NAME/$FNAME --rep 20 --rep-para 16 -P $PREFIX/$NAME.csv
}

mkdir -p $PREFIX

analyze id
analyze uname
analyze cat
analyze mv
analyze ln
analyze date
analyze join

analyze sort
analyze ls a_0.txt


analyze grin
analyze pylint a_0.txt
analyze unison a_0.txt
analyze bibtex2html a_0.txt


analyze vsftpd a_0.txt
analyze ngircd
