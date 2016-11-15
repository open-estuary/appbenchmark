#!/bin/bash

if [ $# -lt 3 ]; then
    echo "Usage: client_start.sh {init | test}  <ip_address> <hadoop_inst_number> <keep_alive> <pipe_num>"
    exit 0
fi

BASE_DIR=$(cd ~; pwd)

REDIS_TEST_DIR=${BASE_DIR}/apptests/hadoop/
REDIS_CMD_DIR=/usr/local/hadoop/bin
#######################################################################################
# Notes:
#  To start client tests
#  Usage: client_start.sh {init | test} <ip_addr> <hadoop_inst> <keep-alive> <pipe_num>
#######################################################################################

ip_addr=${2}
base_port_num=7000
hadoop_inst_num=${3}
keep_alive=${4}
pipeline=${5}

if [ "$1" == "init" ] ; then
    #Step 1: Prepare data
    mkdir -p ${REDIS_TEST_DIR}
    pushd ${REDIS_TEST_DIR} > /dev/null

    data_num=10000
    data_size=128

    python ${APP_ROOT}/apps/hadoop/hadoop_test1/scripts/generate_inputdata.py ./input_data ${data_num} ${data_size}
   
    let "hadoop_inst_num--"
    for index in $(seq 0 ${hadoop_inst_num})
    do
        echo "call hadoop-cli to initialize data for hadoop-${index}"
        port=`expr ${base_port_num} + ${index}`
        cat ./input_data | ${REDIS_CMD_DIR}/hadoop-cli -h ${ip_addr} -p ${port} --pipe
    done

    popd > /dev/null

elif [ "$1" == "test" ] ; then
    pushd ${REDIS_TEST_DIR} > /dev/null
    rm hadoop_benchmark_log*

    let "hadoop_inst_num--"
    for index in $(seq 0 ${hadoop_inst_num})
    do
        port=`expr ${base_port_num} + ${index}`
        taskindex=`expr 31 - ${index}`
        echo "call hadoop-benchmark to test hadoop-${index}"
        taskset -c ${taskindex} ${REDIS_CMD_DIR}/hadoop-benchmark -h ${ip_addr} -p ${port} -c 50 -n 1000000 -d 10 -k ${keep_alive} -r 10000 -P ${pipeline} -t get > hadoop_benchmark_log_${port} &
    done

    echo "Please check results under ${REDIS_TEST_DIR} directory"
    echo "You could use scripts/analysis_qps_lat.py to get qps and latency from logs"

    popd > /dev/null
else 
    echo "parameter should be {init | test} "
fi

echo "**********************************************************************************"

