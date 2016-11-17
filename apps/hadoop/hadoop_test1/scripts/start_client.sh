#!/bin/bash

if [ $# -lt 0 ]; then
    echo "Usage: client_start.sh {all | or specific func name}  <ip_address>"
    exit 0
fi

HIBENCH_CMD_DIR=/usr/local/hibench/
#######################################################################################
# Notes:
#  To start client tests
#######################################################################################

if [ "x${1}" == "xall" ] ; then
    ${HIBENCH_CMD_DIR}/bin/run-all.sh
else 
    echo "Prepare Data ......"
    ${HIBENCH_CMD_DIR}/workloads/${1}/prepare/prepare.sh
 
    echo "Begin to execute benchmark ......"
    ${HIBENCH_CMD_DIR}/workloads/${1}/mapreduce/bin/run.sh
fi

echo "Test Result Report:"
cat ${HIBENCH_CMD_DIR}/report/hibench.report
echo "**********************************************************************************"
