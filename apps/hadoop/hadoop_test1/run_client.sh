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

APP_ROOT="/u01/perfdatas/applications"

#Ip address has been specified by hadoop.conf

if [ $# -lt 1 ] ; then 
    echo "Usage: ./run_test.sh {all or speicfic func name such as wordcount} {data_size=tiny, small, large, huge, gigantic, bigdata}"
    exit 0
fi

if [ ! -z "${2}" ] ; then
    if [ "${2}" != "tiny" ] && [ "${2}" != "small" ] && [ "${2}" != "large" ] && [ "${2}" != "huge" ] && [ "${2}" != "gigantic" ] && [ "${2}" != "bigdata" ]; then
        echo "Usage: ./run_test.sh {all or speicfic func name such as wordcount} {data_size=tiny, small, large, huge, gigantic, bigdata}"
        exit 0
    fi
fi

echo "Start client to test Hadoop server performance ......"

#Include common setup utility functions
NUMA_CMD=""
#NUMA_CMD="numactl --cpunodebind=0,1,2,3 --localalloc"
${NUMA_CMD} ${APP_ROOT}/apps/hadoop/hadoop_test1/scripts/start_client.sh $@
