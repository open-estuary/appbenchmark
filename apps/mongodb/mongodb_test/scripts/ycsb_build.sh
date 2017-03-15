#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build MongoDB-YCSD
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
INSTALL_DIR="/usr/local/ycsb"

####################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/ycsb_data/ycsb-mongodb/workloads/workloada)"  == 0 ]; then
    echo "MongoDB-YCSD has been built successfully"
    exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
#$(tool_add_sudo) rm -fr ${BUILD_DIR}
if [ ! -d ${INSTALL_DIR} ] ; then
    mkdir ${INSTALL_DIR}
fi

tar -zxvf ${SERVER_FILENAME} -C ${INSTALL_DIR}
TARGET_DIR=$(tool_get_first_dirname ${INSTALL_DIR})

######################################################################################
echo "Download database data for mongodb test ..."
pushd ${INSTALL_DIR}
git clone https://github.com/mongodb-labs/YCSB ycsb_data
echo "Finish build preparation......"

popd > /dev/null
##########################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/ycsb_data/ycsb-mongodb/workloads/workloada)"  == 0 ]; then
    echo "Build and install ycsb successfully"
else 
    echo "Fail to build ycsb"
    exit 1
fi
