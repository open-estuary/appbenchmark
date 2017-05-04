#!/bin/bash

#Define global APP_ROOT directory

IMAGE_NAME="foo"
CUR_DIR="$(cd `dirname $0`; pwd)"
${CUR_DIR}/scripts/start_block_device_client.sh  $1 "${IMAGE_NAME}"

BLOCK_DIR_NAME="/mnt/ceph-block-device"
df -h ${BLOCK_DIR_NAME}

TEST="${2}"

if [ -z "${TEST}" ] || [ "${TEST}" == "rbd" ] ; then
    echo "==============================================================================="
    echo "RBD bench test ..."
    IO_SIZE="4M"
    IO_THREADS="64"
    IO_TOTAL="2G"
    #seq or rand
    IO_PATTERN="rand"
    #read or write
    IO_TYPE="write"
    rbd bench --image ${IMAGE_NAME} --io-size ${IO_SIZE} \
          --io-threads ${IO_THREADS} \
          --io-total ${IO_TOTAL} \
          --io-pattern ${IO_PATTERN} \
          --io-type ${IO_TYPE}

else
    echo "==============================================================================="
    echo "FIO Block test ...."
    RUN_TIME=120
    RW_TYPE="write"
    sed -i "s/^rbdname=.*/rbdname=${IMAGE_NAME}/g" ${CUR_DIR}/scripts/fio.test
    sed -i "s/^runtime=.*/runtime=${RUN_TIME}/g" ${CUR_DIR}/scripts/fio.test
    sed -i "s/^rw=.*/rw=${RW_TYPE}/g" ${CUR_DIR}/scripts/fio.test


    fio ${CUR_DIR}/scripts/fio.test


fi
