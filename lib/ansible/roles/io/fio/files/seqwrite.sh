#!/bin/bash

if [ -z "$(which fio 2>/dev/null)" ] ; then
    echo "Please install fio firstly !"
    exit 0
fi
IP=`ifconfig eth0|sed -n 2p|awk  '{ print $2 }'|tr -d 'addr:'`;
CUR_DIR=$(cd `dirname $0`; pwd)
if [ ! -d "${CUR_DIR}/result" ]; then
  mkdir -p  ${CUR_DIR}/result
fi

IOTEST_DIR="${CUR_DIR}/result"
#fio 
filename="/dev/sda2"
direct=1
iodepth=512
rw="write"
ioengine="libaio"
bs="4k"
size="20G"
numjobs=64
runtime=60
name="mytest"

echo "Start fio test......"
pushd ${IOTEST_DIR} > /dev/null
#this for fio test

nohup fio -filename=${filename} -direct=${direct} -iodepth ${iodepth} -thread -rw=${rw} -ioengine=${ioengine} -bs=${bs} -size=${size} -numjobs=${numjobs} -runtime=${runtime} -group_reporting -name=${name} >>result_${rw}_${IP} 2>&1 &

popd > /dev/null

