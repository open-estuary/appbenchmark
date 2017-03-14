#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
${CUR_DIR}/setup.sh client

SERVERIP=${1}

if [ -z "${SERVERIP}" ] ; then
    echo "Usage: ./run_test.sh <remote server IP> "
    exit 0
fi

echo "Start Client Right Now"
qperf -oo msg_size:1:64K:*2  ${SERVERIP} tcp_bw tcp_lat


