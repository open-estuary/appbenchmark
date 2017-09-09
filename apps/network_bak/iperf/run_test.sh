#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
${CUR_DIR}/setup.sh client

SERVERIP=${1}

if [ -z "${SERVERIP}" ] ; then
    echo "Usage: ./run_test.sh <remote server IP> "
    exit 0
fi

echo "Start Client Right Now"
iperf -c ${SERVERIP} -P 1 -t 100 -i 1 -w 256k
iperf -c ${SERVERIP} -P 4 -t 100 -i 1 -w 256k


