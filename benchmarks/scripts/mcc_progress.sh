#!/bin/bash

set -e

IN_PREFIX=res/Analyze_progress$1
OUT_PREFIX=res/Analyze_progress$1

mcc(){
	NAME=$1
	N=$(ls $IN_PREFIX/$NAME | wc -w)
	./igen4 -A0 -T3 -GF 2/$NAME -I res/$NAME/full.txt,$IN_PREFIX/$NAME/iter_{i+1}.txt --rep $N --rep-para 14 -P $OUT_PREFIX/$NAME.csv \
		--params-fields _repeat_id,n_configs,delta_locs,avg_mcc,cnt_exact,cnt_interactions,cnt_wrong,avg_f,_thread_id
}

mkdir -p $OUT_PREFIX

mcc id
mcc uname
mcc cat
mcc mv
mcc ln
mcc date
mcc join

mcc grin
mcc ngircd

#mcc sort
