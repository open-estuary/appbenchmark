#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build fio 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
INSTALL_DIR="/usr/local/fio/"

#######################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/bin/fio)"  == 0 ]; then
      echo "Stream has been installed successfully"
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
# Build fio
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR}/${TARGET_DIR} > /dev/null

#Disable armv8-a+ctc+crypo because the gcc fails to compile it so far
sudo sed -i 's/.*if.*compile_prog.*\"-march=armv8-a+crc+crypto/\ if\ !\ compile_prog\ \"-march=armv8-a+crc+crypto/g' configure 

./configure --prefix=${INSTALL_DIR}
make
make install

echo "**********************************************************************************"
popd > /dev/null

##########################################################################################
echo "Build fio successfully"
