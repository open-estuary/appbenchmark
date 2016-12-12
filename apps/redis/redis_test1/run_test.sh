#!/bin/bash

#Define global APP_ROOT directory

ip="192.168.10.174"

echo "Try to connect server-${ip}......"

test_log_dir="/root/apptests/redis/"

#Bind network interrupt to specific cpus
python ../../../toolset/perftools/miscs/set_ethirq_cpu_affinity.py 0 15

start_cpu_num=20
inst_num=1
echo "Initialize database......"
#./scripts/init_test.sh init ${ip} ${start_cpu_num} ${inst_num} 

echo "Short case"
./scripts/init_test.sh test ${ip} ${start_cpu_num} ${inst_num} 0 1
./scripts/analysis_qps_lat.py ${test_log_dir} ${inst_num}

echo "Basci case"
./scripts/init_test.sh test ${ip} ${start_cpu_num} ${inst_num} 1 1
./scripts/analysis_qps_lat.py ${test_log_dir} ${inst_num}

echo "Pipeline case"
./scripts/init_test.sh test ${ip} ${start_cpu_num} ${inst_num} 1 100
./scripts/analysis_qps_lat.py ${test_log_dir} ${inst_num}

