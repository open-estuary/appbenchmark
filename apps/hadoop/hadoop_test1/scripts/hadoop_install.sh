#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh
######################################################################################
# Notes:
#  To install Hadoop
#
#####################################################################################
BUILD_DIR="./"$(tool_get_build_dir $1)
VERSION="2.6.5"
INSTALL_DIR="/home/hadoop"
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
SERVER_FILENAME=${BUILD_DIR}/${TARGET_DIR}/hadoop-dist/target/hadoop-${VERSION}.tar.gz
#######################################################################################
if [  "$(tool_check_exists ${SERVER_FILENAME})"  == 0 ]; then
      echo "Hadoop-${VERSION} has not been built successfully"
      exit -1
fi

TARGET_DIR=$(tool_get_first_dirname ${INSTALL_DIR})
if [ "$(tool_check_exists ${INSTALL_DIR}/${TARGET_DIR}/bin/hadoop)"  == 0 ]; then
      echo "Hadoop-${VERSION} has been installed successfully"
      exit 0
fi

####################################################################################
# Prepare for install
####################################################################################
$(tool_add_sudo) useradd hadoop
$(tool_add_sudo) passwd hadoop hadooptest
$(tool_add_sudo) mkdir -p ${INSTALL_DIR}
$(tool_add_sudo) chown hadoop.$(whoami) ${INSTALL_DIR}

tar -zxvf ${SERVER_FILENAME} -C ${INSTALL_DIR}
TARGET_DIR=$(tool_get_first_dirname ${INSTALL_DIR})
source /etc/profile
echo "Finish install preparation......"

######################################################################################
# Install Hadoop
######################################################################################

if [ -z "$(grep HADOOP_INSTALL /etc/profile)" ] ; then
    echo "export HADOOP_INSTALL=${INSTALL_DIR}/${TARGET_DIR}" >> /etc/profile
    echo 'export PATH=${HADOOP_INSTALL}/bin:${HADOOP_INSTALL}/sbin:${PATH}' >> /etc/profile
    echo 'export HADOOP_MAPRED_HOME=${HADOOP_INSTALL}' >> /etc/profile
    echo 'export HADOOP_COMMON_HOME=${HADOOP_INSTALL}' >> /etc/profile
    echo 'export HADOOP_HDFS_HOME=${HADOOP_INSTALL}' >> /etc/profile
    echo 'export YARN_HOME=${HADOOP_INSTALL}' >> /etc/profile
    echo 'export HADOOP_COMMON_LIB_NATIVE_DIR=${HADOOP_INSTALL}/lib/native' >> /etc/profile
    echo 'export HADOOP_OPTS="-Djava.library.path=${HADDOP_INSTALL}/lib:${HADOOP_INSTALL}/lib/native"' >> /etc/profile
    echo 'export PATH=$HADOOP_INSTALL/bin:$PATH' >> /etc/profile
fi
source /etc/profile

##########################################################################################
