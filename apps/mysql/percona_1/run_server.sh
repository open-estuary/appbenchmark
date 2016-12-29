#!/bin/bash

#Define global APP_ROOT directory
if [ -z "${APP_ROOT}" ]; then
    # Default value
    APP_ROOT=$(cd `dirname $0` ; cd ../../../; pwd)
else
    # Re-declare so it can be used in this script
    APP_ROOT=$(echo $APP_ROOT)
fi
export APP_ROOT=${APP_ROOT}

./setup.sh server

echo "Start Single Percona Server ......"

${APP_ROOT}/apps/mysql/percona_1/scripts/start_server.sh init
${APP_ROOT}/apps/mysql/percona_1/scripts/start_server.sh start

log_name="./orzdba.log"
if [ ! -z "${1}" ] ; then
    log_name=${1}
fi

echo "Stop irqbalance service firstly"
service irqbalance stop
#Bind network interrupt to specific cpus
python ../../../toolset/perftools/miscs/set_ethirq_cpu_affinity.py 0 7
ps -aux | grep mysqld | grep -v grep | awk '{print $2}' | xargs -n 1 taskset -pac 10-63
echo "Start orzdba and tcprstat to catpure latency ......"
orzdba -mysql -i 10 -rt -c -L ${log_name} > /dev/null 2>&1 &
ps -aux | grep tcprstat | grep -v grep | awk '{print $2}' | xargs -n 1 taskset -pac 8-9
