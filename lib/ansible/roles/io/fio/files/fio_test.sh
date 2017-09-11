#!/bin/bash

if [ -z "$(which fio 2>/dev/null)" ] ; then
    echo "Please install fio firstly !"
    exit 0
fi
if [ -z "$(which ifconfig 2>/dev/null)" ] ; then
    echo "Please install net-tools firstly !"
    yum install net-tools -y
    exit 0
fi
IP=`ifconfig eth0|sed -n 2p|awk  '{ print $2 }'|tr -d 'addr:'`;
CUR_DIR=$(cd `dirname $0`; pwd)
if [ ! -d "${CUR_DIR}/result" ]; then
  mkdir -p  ${CUR_DIR}/result
fi
IOTEST_DIR="${CUR_DIR}/result"
pushd ${IOTEST_DIR} > /dev/null

nohup fio ../fio.cfg  >>result_fiotest_${IP} 2>&1 & 

popd > /dev/null

