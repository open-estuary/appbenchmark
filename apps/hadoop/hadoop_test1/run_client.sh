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


ip="192.168.1.187"
userid="root"
password="123456"

if [ $# -lt 1 ] ; then 
    echo "Usage: ./run_test.sh {init | loaddata | test}"
    exit 0
fi

echo "Start client to test Hadoop server performance ......"

#Include common setup utility functions
${APP_ROOT}/apps/hadoop/hadoop_test1/scripts/start_client.sh ${ip} \
    ${userid} ${password} ${1}
