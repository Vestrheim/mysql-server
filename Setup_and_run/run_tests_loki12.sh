#!/bin/bash

cd /export/home/tmp/bygging

#Comment this part back in to make the ninja files again, should not be needed however.
#cmake /home/svestrhe/Documents/Forprosjekt/code/mysql-server/ -G Ninja -DWITH_BOOST=/usr/global/share  #-DDOWNLOAD_BOOST=1

ninja

rm -r ../datasets

scp -rp svestrhe@atum03:/export/home/tmp/datasets /export/home/tmp

rm /home/svestrhe/Documents/Forprosjekt/code/mysql-server/mysql-test/suite/histogram_plugin/run_data_loki_12.txt


cd mysql-test

./mtr --nocheck-testcase --mem --suite=histogram_plugin --record execute_timing_loki12 --testcase-timeout=300000 --suite-timeout=300000 --mysqld=--plugin_dir=/export/home/tmp/bygging/plugin_output_directory/ --mysqld=--join-buffer-size=524288000 --mysqld=--read-buffer-size=524288000 --mysqld=--sort-buffer-size=524288000 --mysqld=--query-prealloc-size=524288000 --mysqld=--key-buffer-size=4294967296 --mysqld=--innodb-buffer-pool-size=12G --mysqld=--disable_log_bin --mysqld=--performance-schema-events-statements-history-long-size=1048576 --mysqld=--secure-file-priv="" #--mysqld=--innodb-buffer-pool-instances=16

#cd /dev/shm
#rm -r var_*