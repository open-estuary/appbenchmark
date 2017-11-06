#!/bin/bash

yum clean all
yum install qperf -y

qperf_dir="/tmp/dockernet_test/{{ netarch }}/qperf"
rm -rf ${qperf_dir}
mkdir -p ${qperf_dir}

qperf_file="${qperf_dir}/tcp_bw_lat.dat"

touch ${qperf_file}

qperf -oo msg_size:1:64K:*2 {{ netarch }}_qperf_servercon tcp_bw tcp_lat > ${qperf_file}
