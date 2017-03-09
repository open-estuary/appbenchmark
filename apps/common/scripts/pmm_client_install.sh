#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

BASE_DIR="$(cd ~; pwd)"
source ${BASE_DIR}/.bashrc

######################################################################################
# Notes:
#  To build and pmm-client
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

####################################################################################
# Prepare for build
####################################################################################
if [ "$(which pmm-client 2>/dev/null)" ] ; then
    echo "pmm-client has been built, so do nothing"
    echo "Build pmm-client successfully"
    exit 0 
fi

$(tool_add_sudo) rm -fr ${BUILD_DIR}/${TARGET_DIR}*
mkdir -p ${BUILD_DIR}
tar -zxvf ${SERVER_FILENAME} -C ${BUILD_DIR}
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

echo "Finish build preparation......"
######################################################################################
# Build pmm-client
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/

sed -i 's/x86_64/aarch64/g' ./scripts/*
sed -i 's/amd64/aarch64/g' ./scripts/*


$(tool_add_sudo) make install

popd > /dev/null

echo "Build pmm-client completed"

