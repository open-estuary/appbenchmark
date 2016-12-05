#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build and install libaio 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

#######################################################################################
if [ $(tool_check_exists /usr/lib64/libaio.so)  == 0 ] || [ $(tool_check_exists /usr/lib/libaio.so) == 0 ] ; then
      echo "Libaio has been installed successfully"
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
# Build libaio
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/
make 
$(tool_add_sudo) make install

##########################################################################################
echo "Build libaio successfully"
