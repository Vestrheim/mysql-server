#!/bin/bash

cd /export/home/tmp/bygging

#Comment this part back in to make the ninja files again, should not be needed however.
#cmake /home/svestrhe/Documents/Forprosjekt/code/mysql-server/ -G Ninja -DWITH_BOOST=/usr/global/share  -DDOWNLOAD_BOOST=1  -DWITH_DEBUG=1 -DMYSQL_MAINTAINER_MODE=0

ninja

cd mysql-test

./mtr --start --mem --mysqld=--plugin_dir=/export/home/tmp/bygging/plugin_output_directory/ --nocheck-testcase --suite=histogram_plugin --mysqld=--plugin_dir=/export/home/tmp/bygging/plugin_output_directory/ --mysqld=--join-buffer-size=104857600 --mysqld=--read-buffer-size=104857600 --mysqld=--sort-buffer-size=104857600 --mysqld=--query-prealloc-size=104857600 --mysqld=--key-buffer-size=536870912


cd /dev/shm
rm -r var_*