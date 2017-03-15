#!/bin/bash

#Define global APP_ROOT directory

if [ -z "${1}" ] ; then
    echo "Try to connect local host(Note:Use ./run_test.sh <server IP> <pgbench> to connect non-local server"
    IP="127.0.0.1"
else
    IP="${1}"
    echo "Try to connect server-${IP}......"
fi

PORT="27017"
THREADS=32

source /etc/profile

CUR_DIR="$(cd `dirname $0`; pwd)"
${CUR_DIR}/scripts/start_client.sh ${IP} ${PORT} ${THREADS}

