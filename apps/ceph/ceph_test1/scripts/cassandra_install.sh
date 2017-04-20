#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build Ceph
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
INSTALL_DIR="/usr/local/"

####################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/ceph/bin/cqlsh)"  == 0 ]; then
    echo "Ceph has been built successfully"
    exit 0
fi

####################################################################################
# Prepare for install
####################################################################################
if [ -d ${INSTALL_DIR}/ceph ] ; then
    rm -fr ${INSTALL_DIR}
fi

tar -zxvf ${SERVER_FILENAME} -C ${INSTALL_DIR}
mv ${INSTALL_DIR}/apache-ceph-* ${INSTALL_DIR}/ceph

##########################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/ceph/bin/cqlsh)"  == 0 ]; then
    echo "Build and install ceph successfully"
else 
    echo "Fail to build ceph"
    exit 1
fi
