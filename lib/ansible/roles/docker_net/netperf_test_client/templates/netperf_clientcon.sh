#!/bin/bash

yum clean all
yum install netperf -y

netperf_dir="/tmp/dockernet_test/{{ netarch }}/netperf"
rm -rf ${netperf_dir}
mkdir -p ${netperf_dir}

tcpstream_file="${netperf_dir}/netperf_TCP_STREAM.dat"
tcprr_file="${netperf_dir}/netperf_TCP_RR.dat"
tcpcrr_file="${netperf_dir}/netperf_TCP_CRR.dat"
udprr_file="${netperf_dir}/netperf_UDP_RR.dat"

touch ${tcpstream_file}
touch ${tcprr_file}
touch ${tcpcrr_file}
touch ${udprr_file}

netperf -t TCP_STREAM -H netperf_servercon -l 60 -- -m 2048 > ${tcpstream_file}

netperf -t TCP_RR -H netperf_servercon -l 60 -- -r 64,1024 >  ${tcprr_file}

netperf -t TCP_CRR -H netperf_servercon -l 60 -- -r 64,1024 > ${tcpcrr_file}

netperf -t UDP_RR -H netperf_servercon -l 60 -- -r 64,1024 > ${udprr_file}
