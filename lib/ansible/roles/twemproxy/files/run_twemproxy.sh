#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: run_twemproxy.sh <twemproxy_cpus>"
    exit 0
fi

TWEMPROXY_CPUS=${1}

START_CPU_NUM=${TWEMPROXY_CPUS%%-*}
END_CPU_NUM=${TWEMPROXY_CPUS##*-}

TWEMPROXY_CMD_DIR=/usr/sbin
#TWEMPROXY_CFG_DIR=${TWEMPROXY_CMD_DIR}/conf
TWEMPROXY_CFG_DIR=/etc

pushd ${TWEMPROXY_CMD_DIR} > /dev/null

while (( ${START_CPU_NUM} <= ${END_CPU_NUM} ))
do
    portid=`expr 22200 + ${START_CPU_NUM}`
    
    sudo \cp /etc/twemproxy_${START_CPU_NUM}.yml /etc/twemproxy.yml
    echo "Try to start twemproxy associated with cpu  ${START_CPU_NUM} and port ${portid}"
    taskset -c ${START_CPU_NUM} ${TWEMPROXY_CMD_DIR}/twemproxy ${TWEMPROXY_CFG_DIR}/twemproxy_${START_CPU_NUM}.yml -v 4 -m 1024 -s ${portid} &

    let "START_CPU_NUM++"
done

popd > /dev/null
