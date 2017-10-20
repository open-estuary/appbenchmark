#!/bin/bash

yum clean all
yum install iperf -y

iperf_dir="/tmp/dockernet_test/{{ netarch }}/iperf"
rm -rf ${iperf_dir}
mkdir -p ${iperf_dir}

thread1_file="${iperf_dir}/iperf_1thread.dat"
thread4_file="${iperf_dir}/iperf_4thread.dat"

touch ${thread1_file}
touch ${thread4_file}

iperf -c iperf_servercon -P 1 -t 100 -i 1 -w 256k > ${thread1_file}

iperf -c iperf_servercon -P 4 -t 100 -i 1 -w 256k > ${thread4_file}
