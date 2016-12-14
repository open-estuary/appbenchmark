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


echo 1 > /proc/sys/net/ipv4/tcp_timestamps
echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse
echo 1 > /proc/sys/net/ipv4/tcp_tw_recycle
echo 2048 65000 > /proc/sys/net/ipv4/ip_local_port_range
echo 2621440 > /proc/sys/net/core/somaxconn
echo 2621440 > /proc/sys/net/core/netdev_max_backlog
echo 2621440 > /proc/sys/net/ipv4/tcp_max_syn_backlog
echo 1 > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_time_wait
echo 2621440 > /proc/sys/net/netfilter/nf_conntrack_max

#support maxinum number of files open
ulimit -n 102400

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

