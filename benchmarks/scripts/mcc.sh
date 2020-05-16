#!/bin/bash

set -e

PREFIX=res/Analyze/mcc

mcc(){
	NAME=$1
	./igen4 -A0 -T3 -GF 2/$NAME -I res/$NAME/full.txt,res/$NAME/a_{i}.txt --rep 11 --rep-para 11 -P $PREFIX/$NAME.csv \
		--params-fields _repeat_id,_thread_id,delta_locs,avg_mcc,cnt_exact,cnt_interactions,cnt_wrong,avg_f,n_configs,wrong_locs
}

mkdir -p $PREFIX

mcc id
mcc uname
mcc cat
mcc mv
mcc ln
mcc date
mcc join

mcc sort
#mcc ls


mcc grin
#mcc pylint
#mcc unison
#mcc bibtex2html


#mcc vsftpd
mcc ngircd
