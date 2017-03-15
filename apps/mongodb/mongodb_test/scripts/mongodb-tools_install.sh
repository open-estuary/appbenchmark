#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build MongoDB
#
#####################################################################################

BUILD_DIR="./builddir_mongotools"
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
INSTALL_DIR="/usr/local/mongo-tools"

#######################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/bin/mongotop)"  == 0 ]; then
      echo "MongoDB-Tools has been installed successfully"
      exit 0
fi

####################################################################################
# Prepare for install
####################################################################################
if [ ! -d ${INSTALL_DIR}/bin ] ; then
    mkdir -p ${INSTALL_DIR}/bin
fi

if [ ! -d ${BUILD_DIR} ] ; then
    mkdir -p ${BUILD_DIR}
fi

pushd ${BUILD_DIR} > /dev/null
git clone https://github.com/mongodb/mongo-tools.git
cd mongo-tools

./build.sh

cp -fr ./bin/* ${INSTALL_DIR}/bin/

popd > /dev/null

if [ "$(tool_check_exists ${INSTALL_DIR}/bin/mongotop)"  == 0 ]; then
    echo "Install Mongo-Tools successfully"
else 
    echo "Fail to build Pgbench-tools"
    exit 1
fi
