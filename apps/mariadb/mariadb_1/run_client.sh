#!/bin/bash

if [ $# -lt 1 ] ; then 
    echo "Usage: ./run_client.sh {ip}"
    exit 0
fi

ip=${1}

CUR_DIR=$(cd `dirname $0`; pwd)

${CUR_DIR}/setup.sh client

${CUR_DIR}/scripts/init_client.sh ${ip} init
${CUR_DIR}/scripts/init_client.sh ${ip} loaddata

