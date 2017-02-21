#!/bin/bash

if [ "$(whoami)" == "root" ] ; then
    echo "It must use non-root to execute these packages"
    exit 0
fi

CUR_DIR=$(cd `dirname $0`; pwd)
sudo ${CUR_DIR}/setup.sh server

default_scale="small"
if [ ! -z "${1}" ] ; then
    if [ "${1}" != "large" ]  && [ "${1}" != "small" ] ; then
        echo "./run_server.sh {large(default) | small}"
        exit 0
    fi
    scale=${1}
else 
    scale=${default_scale}
fi

${CUR_DIR}/scripts/start_server.sh ${scale}

sleep 10
if [ -z "$(ps -aux | grep postgres | grep -v grep)" ] ; then
    echo "*******************************************************************************************"
    echo "Probaly there is no enough memory, so try to use small configuration such as ./run_server.sh small"
fi
