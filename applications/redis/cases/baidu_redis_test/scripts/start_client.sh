#!/bin/bash

if [ $# -lt 3 ]; then
    echo "Usage: client_start.sh <ip_address> <redis_inst_number> {init | test} {pipe_num}"
    exit 0
fi

BASE_DIR=$(cd ~; pwd)

REDIS_TEST_DIR=${BASE_DIR}/apptests/redis/
REDIS_CMD_DIR=/usr/local/redis/bin
######################################################################################
# Notes:
#  To start client tests
#  Usage: client_start.sh <redis_ip> <redis_inst_number> {init | test} {pipe_num}
#####################################################################################

ip_addr=${1}
base_port_num=7000
rediss_inst_num=${2}

if [ "$3" == "init" ] ; then
    #Step 1: Prepare data
    mkdir -p ${REDIS_TEST_DIR}
    pushd ${REDIS_TEST_DIR} > /dev/null

    python ${APP_ROOT}/pplications/redis/cases/baidu_redis_test/scripts/generate_inputdata.py ./input_data
    
    for index in $(seq 0 ${redis_inst_num})
    do
        port=`expr ${base_port_num} + ${index}`
        cat ./input_data | ${REDIS_CMD_DIR}/redis-cli -h ${ip_addr} -p ${port} --pipe
    done

    popd > /dev/null

elif [ "$3" == "test" ] ; then
    pushd ${REDIS_TEST_DIR} > /dev/null

    pipeline=100
    if [ "$4" ] ; then
        pipeline=${4}
    fi

    for index in $(seq 0 ${redis_inst_num})
    do
        port=`expr ${base_port_num} + ${index}`
        taskset -c ${index} ${REDIS_CMD_DIR}/redis-benchmark -h ${ip_addr} -p ${port} -c 50 -n 10000000 -d 10 -k 1 -r 10000 -P ${pipeline} -t get > redis_pipeline_$2_${port} &
    done

    popd > /dev/null
else 
    echo "parameter should be {init | test} "
fi

echo "**********************************************************************************"

