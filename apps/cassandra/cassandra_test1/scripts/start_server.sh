#!/bin/bash

BASE_DIR=$(cd ~; pwd)
CASSANDRA_HOME=/usr/local/cassandra
BASE_DIR="/u01/cassandra"
DATA_DIR=${BASE_DIR}/data
COMMIT_DIR=${BASE_DIR}/commitlog
CACHES_DIR=${BASE_DIR}/saved_caches
HITS_DIR=${BASE_DIR}/hints
TMP_DIR=${BASE_DIR}/tmp

CUR_DIR=$(cd `dirname $0`; pwd)

##################################################################################################
# Define system variables 
##################################################################################################
source /etc/profile

##################################################################################################
# Initialize database 
##################################################################################################

# Make sure that file system have been formatted and mounted properly
# 
# DISK=/dev/sda
# parted -s ${DISK} mklabel gpt
# parted -s ${DISK} mkpart primary 1MiB 100%
# mkfs.ext4 /dev/sda1 -m 0 -O extent,uninit_bg -E lazy_itable_init=1 -T largefile -L u01
# 
# vim /etc/fstab (mount)
# LABEL=u01 /u01     ext4        defaults,noatime,nodiratime,nodelalloc,barrier=0,data=writeback    0 0
#

if [ ! -d "/u01" ] ; then
    echo "Please mount disk to /u01 directory firstly" 
    exit 0
fi

if [ ! -d ${BASE_DIR} ] ; then
    sudo mkdir -p ${BASE_DIR}
fi

sudo chown `whoami` ${BASE_DIR}

if [ ! -d ${DATA_DIR} ] ; then
    mkdir -p ${DATA_DIR}
fi

if [ ! -d ${COMMIT_DIR} ] ; then
    mkdir -p ${COMMIT_DIR}
fi

if [ ! -d ${CACHES_DIR} ] ; then
    mkdir -p ${CACHES_DIR}
fi

if [ ! -d ${HITS_DIR} ] ; then
    mkdir -p ${HITS_DIR}
fi

if [ ! -d ${TMP_DIR} ] ; then
    mkdir -p ${TMP_DIR}
fi

sudo cp ${CUR_DIR}/../config/cassandra.yaml ${CASSANDRA_HOME}/conf/
sudo cp ${CUR_DIR}/../config/jvm.options ${CASSANDRA_HOME}/conf/
sudo cp ${CUR_DIR}/../config/cassandra-env.sh ${CASSANDRA_HOME}/conf/
sudo cp ${CUR_DIR}/../config/logback.xml ${CASSANDRA_HOME}/conf/
sudo sed -i "s/\${cassandra\.logdir}/\/u01\/cassandra\/logs/g" ${CASSANDRA_HOME}/conf/logback.xml

#Repleace JNA libs to support AARCH64 platform
sudo cp ${CUR_DIR}/../source/jna.aarch64.jar ${CASSANDRA_HOME}/lib/jna-*.jar

${CASSANDRA_HOME}/bin/cassandra -p ${BASE_DIR}/cassandra.pid

echo "Cassandra has been started in /u01/cassandra directory ..."
echo "**********************************************************************************"

