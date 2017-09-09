#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build Lmbench 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
INSTALL_DIR="/usr/local"

#######################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/bin/iperf)"  == 0 ]; then
      echo "Iperf has been installed successfully"
      exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
$(tool_add_sudo) rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
tar -zxvf ${SERVER_FILENAME} -C ${BUILD_DIR}
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
echo "Finish build preparation......"

######################################################################################
# Build iperf
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR}/${TARGET_DIR} > /dev/null
./configure --prefix=${INSTALL_DIR}  --build=aarch64-unknown-linux-gnu
make
make install

echo "**********************************************************************************"
popd > /dev/null

##########################################################################################
echo "Build iperf successfully"
