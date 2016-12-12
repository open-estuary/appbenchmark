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

echo "Try to connect server-${ip}......"

if [ $# -lt 2 ] ; then 
    echo "Usage: ./init_test.sh {init | test} ${ip} {start_cpu_num} {redis-inst:1 ~ 32} {keep-alive:0 or 1} {pipeline:0 ~ 100}"
    exit 0
fi

#Include common setup utility functions
${APP_ROOT}/apps/redis/redis_test1/scripts/start_client.sh $@
