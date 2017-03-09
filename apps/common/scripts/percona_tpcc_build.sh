#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build tpcc-mysql
#
#####################################################################################

BUILD_DIR="/usr/local/percona-tpcc-mysql"
TARGET_DIR="master"

####################################################################################
# Prepare for build
####################################################################################
if [ $(tool_check_exists "${BUILD_DIR}/tpcc-mysql/tpcc_start") == 0 ] ; then
    echo "tpcc-mysql has been built, so do nothing"
    echo "Build tpcc-mysql successfully"
    exit 0 
fi

if [ ! -d ${BUILD_DIR} ] ; then
    mkdir -p ${BUILD_DIR}
fi

######################################################################################
# Build Tpcc-mysql
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/nulli
cd ${TARGET_DIR}/

git clone https://github.com/Percona-Lab/tpcc-mysql.git
cd tpcc-mysql
cd src
make

popd > /dev/null

echo "**********************************************************************************"
echo "Build tpcc-mysql completed"

