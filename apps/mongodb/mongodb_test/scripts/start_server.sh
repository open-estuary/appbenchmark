#!/bin/bash

export LD_LIBRARY_PATH=/usr/local/lib64:/usr/local/lib:${LD_LIBRARY_PATH}
MONGODBHOME=/usr/local/mongodb
DATA_DIR="/u01/mongodb/data"
REPAIR_DIR=${DATA_DIR}/repair
LOG_DIR="/u01/mongodb/log"


##########################################################################
#1. Turn off HugePage/Defrag and NUMA
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo 0 > /proc/sys/kernel/numa_balancing

#2. Readahead was reduced to 64 blocks
echo "It is assumed that the data will be stored into /dev/sda"
echo "If not, please change device name appropriately"

blockdev --setra 64 /dev/sda

###########################################################################
# Setup Config
###########################################################################
CUR_DIR=$(cd `dirname $0`; pwd)
sudo cp ${CUR_DIR}/../config/mongod.conf /etc/

if [ ! -d ${REPAIR_DIR} ] ; then
    mkdir -p ${REPAIR_DIR}
fi

if [ ! -d ${LOG_DIR} ] ; then
    mkdir -p ${LOG_DIR}
fi

##########################################################################
# Start Server
##########################################################################
numactl --interleave=all ${MONGODBHOME}/bin/mongod -f /etc/mongod.conf
