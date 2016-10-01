#!/bin/bash

#Define global APP_ROOT directory
if [ -z "${APP_ROOT}" ]; then
    # Default value
    APP_ROOT=$(cd `dirname $0` ; cd ../../../../; pwd)
else
    # Re-declare so it can be used in this script
    APP_ROOT=$(echo $APP_ROOT)
fi
export APP_ROOT=${APP_ROOT}


ip="192.168.1.187"

if [ $# -lt 2 ] ; then 
    echo "Usage: ./run_test.sh {init | test} {redis-inst:1 ~ 32} {keep-alive:0 or 1} {pipeline:0 ~ 100}"
    exit 0
fi

#Include common setup utility functions
${APP_ROOT}/apps/redis/baidu_redis_test/scripts/start_client.sh ${1} ${ip} ${2} ${3}
