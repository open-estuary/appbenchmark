#!/bin/bash

if [ -z "$(which fio 2>/dev/null)" ] ; then
    echo "Please install sysbench firstly !"
    exit 0
fi
if [ -z "$(which ifconfig 2>/dev/null)" ] ; then
    echo "Please install net-tools firstly !"
    yum install net-tools -y
    exit 0
fi

IP=`ifconfig eth0|sed -n 2p|awk  '{ print $2 }'|tr -d 'addr:'`;
CUR_DIR=$(cd `dirname $0`; pwd)
if [ -d "${CUR_DIR}/result" ]; then
  rm -rf ${CUR_DIR}/result
fi
SLEEPTIME=315
pushd ${CUR_DIR} > /dev/null
sh rndread.sh 
sleep ${SLEEPTIME} 
sh rndrw.sh
sleep ${SLEEPTIME} 
sh  rndwrite.sh
sleep ${SLEEPTIME}
sh seqread.sh 
sleep ${SLEEPTIME}
sh seqwrite.sh
sleep ${SLEEPTIME}
popd > /dev/null







