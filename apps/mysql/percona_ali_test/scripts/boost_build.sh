#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build Boost-Server 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})


#######################################################################################
if [ "$(tool_check_exists lib/boost)"  == 0 ]; then
      echo "Boost has been built successfully"
      exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
$(tool_add_sudo) rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
tar -zxvf ${SERVER_FILENAME} -C ${BUILD_DIR}
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

echo "Build_dir:${BUILD_DIR}, Target:${TARGET_DIR}"
echo "Finish build preparation......"

######################################################################################
# Build boost
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/tools/build
./bootstrap.sh
./b2 install --prefix=/usr/local/boost
cd ../..
b2 --build-dir=../build-boost toolset=gcc stage

if [ -z "$(grep "/usr/local/boost" ~/.bashrc)" ] ; then
    echo "PATH=/usr/local/boost/bin/:$PATH" >> ~/.bashrc
    source ~/.bashrc
fi

##########################################################################################
echo "Build boost successfully"
