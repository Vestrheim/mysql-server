#!/bin/bash

cd /export/home/tmp/bygging

#Comment this part back in to make the ninja files again, should not be needed however.
#cmake /home/svestrhe/Documents/Forprosjekt/code/mysql-server/ -G Ninja -DWITH_BOOST=/usr/global/share  #-DDOWNLOAD_BOOST=1

ninja

cd mysql-test

rm suite/histogram_plugin/run_data.txt

./mtr --nocheck-testcase --mem --suite=histogram_plugin --record execute_timing_local --testcase-timeout=30000 --suite-timeout=30000 --mysqld=--plugin_dir=/export/home/tmp/bygging/plugin_output_directory/ --mysqld=--join-buffer-size=104857600 --mysqld=--read-buffer-size=104857600 --mysqld=--sort-buffer-size=104857600 --mysqld=--query-prealloc-size=104857600 --mysqld=--key-buffer-size=1073741824 --mysqld=--innodb-buffer-pool-size=6G --mysqld=--disable_log_bin --mysqld=--performance-schema-events-statements-history-long-size=1048576 --mysqld=--secure-file-priv="" #--mysqld=--innodb-buffer-pool-instances=16

cd /dev/shm
rm -r var_*