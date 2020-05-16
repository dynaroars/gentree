#!/bin/bash

set -e

PREFIX=res/Analyze/stat

analyze(){
	NAME=$1
	./igen4 -A0 -T2 -GF 2/$NAME -I res/$NAME/a_{i}.txt --rep 11 --rep-para 11 -P $PREFIX/$NAME.csv \
		--params-fields _repeat_id,_thread_id,n_configs,n_locs,t_search,t_total,cnt_singular,cnt_and,cnt_or,cnt_mixed,cnt_total,cnt_pure,cnt_compat,hash,iter,n_cache_hit,n_locs_total,n_locs_uniq,repeat_id,t_runner,t_runner_total
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
analyze ls


analyze grin
analyze pylint
analyze unison
analyze bibtex2html


analyze vsftpd
analyze ngircd
