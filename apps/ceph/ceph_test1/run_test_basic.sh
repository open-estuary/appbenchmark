#!/bin/bash

echo "Ceph basic benchmark test....."
CUR_DIR="$(cd `dirname $0`; pwd)"

POOL_NAME="rbd"
SECONDS=60

#valid type: write, seq, rand
if [ -z "${1}" ] ; then
    TEST_TYPE="write"
else 
    TEST_TYPE="${1}"
fi

BLOCK_SIZE="4M"
THREADS="64"

echo "Rados bench test...."
rados bench -p ${POOL_NAME} ${SECONDS} ${TEST_TYPE} -b ${BLOCK_SIZE} -t ${THREADS} 


echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
echo "Rados load-gen test..."
NUM_OBJECTS=10240
MIN_SIZE="4M"
MAX_SIZE="32M"
MAX_OPS=16
MIN_OP_LEN="4M"
MAX_OP_LEN="32M"

#Read operation percent
PERCENT="50"  
TARGET_THROUGHTPUT=2000
#Total run time in seconds
RUN_LENGTH=120

rados -p ${POOL_NAME} load-gen --num-objects ${NUM_OBJECTS} \
      --min-object-size ${MIN_SIZE} \
      --max-object-size ${MAX_SIZE} \
      --max-ops ${MAX_OPS} \
      --min-op-len ${MIN_OP_LEN} \
      --max-op-len ${MAX_OP_LEN} \
      --percent ${PERCENT} \
      --target-throughtput ${TARGET_THROUGHTPUT} \
      --run-length ${RUN_LENGTH}


