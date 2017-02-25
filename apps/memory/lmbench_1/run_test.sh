#!/bin/bash

if [ -z "$(which stream 2>/dev/null)" ] ; then
    echo "Please install stream firstly !"
    exit 0
fi

CUR_DIR=$(cd `dirname $0`; pwd)

if [ -z "$(echo $PATH | grep '/usr/local/bin')" ] ; then
    export PATH=$PATH:/usr/local/bin
fi

CPUCORES=`lscpu | grep "^CPU(s):" | awk '{$2 = $2 + 0; print $2}'`

${CUR_DIR}/scripts/lmbench_bw.sh ${CPUCORES}
${CUR_DIR}/scripts/lmbench_latency.sh ${CPUCORES}



