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

echo "Begin to start Hadoop Server ......"

rm -fr /u01/hadoop/tmp/*
rm -fr /u01/hadoop/hadoop-2.6.0/logs/*

ulimit -n 32768
#sysctl -p
service irqbalance stop
echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo cfq > /sys/block/sda/queue/scheduler

#Include common setup utility functions
${APP_ROOT}/apps/hadoop/hadoop_test1/scripts/start_server.sh $@ 
