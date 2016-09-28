#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage: client_start.sh <start_cpu_num> <end_cpu_num>"
    exit 0
fi

BASE_DIR=$(cd ~; pwd)
REDIS_TEST_DIR=${BASE_DIR}/apptests/redis/
REDIS_CMD_DIR=/usr/local/redis/bin
REDIS_CFG_DIR=/usr/local/redis/config

mkdir -p ${REDIS_TEST_DIR}
pushd ${REDIS_TEST_DIR} > /dev/null

start_cpu_num=${1}
end_cpu_num=${2}


if [ ${start_cpu_num} -gt ${end_cpu_num} ] ; then
    echo "the start_cpu_num should be less than end_cpu_num"
    exit 0
fi

redis_inst=0
while (( ${start_cpu_num} <= ${end_cpu_num} ))
do
    portid=`expr 7000 + ${start_cpu_num}`
    echo "Try to start redis-server associated with cpu${start_cpu_num} and port-${portid}"
    taskset -c ${start_cpu_num} ${REDIS_CMD_DIR}/redis-server ${REDIS_CFG_DIR}/redis_cpu${start_cpu_num}_port${portid}.conf

    let "start_cpu_num++"
    let "redis_inst++"
done

popd > /dev/null
echo "${redis_inst} redis-servers have been started"

echo "**********************************************************************************"

