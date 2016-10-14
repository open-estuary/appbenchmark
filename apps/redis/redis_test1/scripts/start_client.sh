#!/bin/bash

if [ $# -lt 3 ]; then
    echo "Usage: client_start.sh {init | test}  <ip_address> <redis_inst_number> <keep_alive> <pipe_num>"
    exit 0
fi

BASE_DIR=$(cd ~; pwd)

REDIS_TEST_DIR=${BASE_DIR}/apptests/redis/
REDIS_CMD_DIR=/usr/local/redis/bin
#######################################################################################
# Notes:
#  To start client tests
#  Usage: client_start.sh {init | test} <ip_addr> <redis_inst> <keep-alive> <pipe_num>
#######################################################################################

ip_addr=${2}
base_port_num=7000
redis_inst_num=${3}
keep_alive=${4}
pipeline=${5}

if [ "$1" == "init" ] ; then
    #Step 1: Prepare data
    mkdir -p ${REDIS_TEST_DIR}
    pushd ${REDIS_TEST_DIR} > /dev/null

    data_num=10000
    data_size=128

    python ${APP_ROOT}/apps/redis/redis_test1/scripts/generate_inputdata.py ./input_data ${data_num} ${data_size}
   
    let "redis_inst_num--"
    for index in $(seq 0 ${redis_inst_num})
    do
        echo "call redis-cli to initialize data for redis-${index}"
        port=`expr ${base_port_num} + ${index}`
        cat ./input_data | ${REDIS_CMD_DIR}/redis-cli -h ${ip_addr} -p ${port} --pipe
    done

    popd > /dev/null

elif [ "$1" == "test" ] ; then
    pushd ${REDIS_TEST_DIR} > /dev/null
    rm redis_benchmark_log*

    let "redis_inst_num--"
    for index in $(seq 0 ${redis_inst_num})
    do
        port=`expr ${base_port_num} + ${index}`
        echo "call redis-benchmark to test redis-${index}"
        taskset -c ${index} ${REDIS_CMD_DIR}/redis-benchmark -h ${ip_addr} -p ${port} -c 50 -n 1000000 -d 10 -k ${keep_alive} -r 10000 -P ${pipeline} -t get > redis_benchmark_log_${port} &
    done

    echo "Please check results under ${REDIS_TEST_DIR} directory"
    echo "You could use scripts/analysis_qps_lat.py to get qps and latency from logs"

    popd > /dev/null
else 
    echo "parameter should be {init | test} "
fi

echo "**********************************************************************************"

