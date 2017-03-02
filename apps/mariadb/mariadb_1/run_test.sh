#!/bin/bash

if [ -z "${1}" ] ; then 
    echo "Usage: ./run_test.sh {mariadb_server_IP}"
    exit 0
fi

IP=${1}

CUR_DIR="$(cd `dirname $01`; pwd)"

echo 1 > /proc/sys/kernel/numa_balancing

echo "You might need to change IRQCPUS and MYSQLCPUS based on real cpu cores numbers"
IRQCPUS="0 7"
MYSQLCPUS="8-63"

PORT=3306
USER="mysql"
PASSWORD="123456"

##########################################################################
## Test Setup ############################################################
##########################################################################
${CUR_DIR}/run_client.sh ${IP} 
${CUR_DIR}/scrIPts/start_client.sh ${IP} ${USER} ${PASSWORD} init
${CUR_DIR}/scrIPts/start_client.sh ${IP} ${USER} ${PASSWORD} loaddata


##########################################################################
## Execute Test
##########################################################################
PORT=3306
USER="mysql"
PASSWORD="123456"

#Each interation will add 5000 connections
MAX_ITERNUM=6

service irqbalance stop
#Bind network interrupt to specific cpus
python ../../../toolset/perftools/miscs/set_ethirq_cpu_affinity.py 0 7

index=1
while [[ ${index} -lt ${MAX_ITERNUM} ]] 
do

${CUR_DIR}/scrIPts/start_client.sh ${IP} ${USER} ${PASSWORD} test ${index} ${PORT}

ps -aux | grep sysbench | grep -v grep | awk '{print $2}' | xargs -n 1 taskset -pc ${MYSQLCPUS}
ps -aux | grep run | grep -v grep | awk '{print $2}' | xargs -n 1 taskset -pc ${MYSQLCPUS}

sleep 3600
let "index++"
done

