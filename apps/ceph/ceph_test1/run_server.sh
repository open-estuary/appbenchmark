#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
sudo ${CUR_DIR}/setup.sh server

MONITOR_NODE=$1
if [ -z "${1}" ] ; then
    MONITOR_NODE=$(hostname)
fi

is_signle_node="yes"
if [ ! -z "${2}" ] ; then
    is_signle_node="${2}"
fi

${CUR_DIR}/scripts/start_server.sh  "${MONITOR_NODE}" "${is_signle_node}"

