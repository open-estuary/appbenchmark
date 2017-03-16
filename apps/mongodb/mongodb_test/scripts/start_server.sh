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
echo 0 > /proc/sys/vm/zone_reclaim_mode

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
enable_auth=0

if [ ${enable_auth} -eq 0 ] ; then
  ##########################################################################
  # Just disable auth during performance test because YCSB doesn't support that yet
  ##########################################################################
  sed -i 's/authorization:\.*enabled/authorization:\ disabled/g' /etc/mongod.conf
  numactl --interleave=all ${MONGODBHOME}/bin/mongod -f /etc/mongod.conf
else
  #Disabled during first startup
  sed -i 's/authorization:\ .*enabled/authorization:\ disabled/g' /etc/mongod.conf
  sudo cp ${CUR_DIR}/../config/mongod.conf /etc/
  numactl --interleave=all ${MONGODBHOME}/bin/mongod -f /etc/mongod.conf

  #Setup root account
  ${MONGODBHOME}/bin/mongo admin ${CUR_DIR}/createroot.js

  echo "Start MongoDB with Auth (root, Estuary12#$)..."
  sed -i 's/authorization:\ disabled/authorization:\ enabled/g' /etc/mongod.conf
  numactl --interleave=all ${MONGODBHOME}/bin/mongod -f /etc/mongod.conf
fi

