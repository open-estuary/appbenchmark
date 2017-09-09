#!/bin/bash

SERVERIP=${1}

if [ -z "${SERVERIP}" ] ; then
    echo "Usage: ./run_test.sh <remote server IP> "
    exit 0
fi

echo "Start Client Right Now"
netperf -t TCP_STREAM -H ${SERVERIP} -l 60 -- -m 2048
netperf -t TCP_RR -H ${SERVERIP} -l 60 -- -m 1024
netperf -t TCP_CRR -H ${SERVERIP} -l 60 -- -m 1024
netperf -t UDP_RR -H ${SERVERIP} -l 60 -- -m 1024


