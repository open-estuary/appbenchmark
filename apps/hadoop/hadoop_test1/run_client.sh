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


#Ip address has been specified by hadoop.conf

if [ $# -lt 1 ] ; then 
    echo "Usage: ./run_test.sh {all or speicfic func name such as wordcount}"
    exit 0
fi

echo "Start client to test Hadoop server performance ......"

#Include common setup utility functions
${APP_ROOT}/apps/hadoop/hadoop_test1/scripts/start_client.sh ${1}
