#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage: run_twemproxy.sh <start_cpu_num> <end_cpu_num>"
    exit 0
fi

start_cpu_num=${1}
end_cpu_num=${2}

APP_CUR_DIR=$(cd `dirname $0` ; pwd)

TWEMPROXY_INSTALL_DIR=/usr/sbin
TWEMPROXY_CFG_DIR=${TWEMPROXY_INSTALL_DIR}/conf

############################
# Notes:
# Copy twemproxy
#############################
mkdir -p ${TWEMPROXY_INSTALL_DIR}/conf
cp ${APP_CUR_DIR}/config/nutcracker* ${TWEMPROXY_INSTALL_DIR}/conf/

##############################
# Notes:
# run twemproxy
#############################

pushd ${TWEMPROXY_INSTALL_DIR} > /dev/null

echo "begin to run twemproxy"

while (( ${start_cpu_num} <= ${end_cpu_num} ))
do
    twemproxy_portid=`expr 22200 + ${start_cpu_num}`
    taskset -c ${start_cpu_num} ${TWEMPROXY_INSTALL_DIR}/twemproxy -d -c ${TWEMPROXY_CFG_DIR}/nutcracker_${start_cpu_num}.yml -v 4 -s ${twemproxy_portid} -m 1024 
    # taskset -c 34 /usr/sbin/twemproxy -d -c /usr/sbin/conf/nutcracker_34.yml -v 4 -m 1024 -s 22234
    
    let "start_cpu_num++"
done

echo "twemproxy is running"

popd > /dev/null
