#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

BASE_DIR="$(cd ~; pwd)"
source ${BASE_DIR}/.bashrc

######################################################################################
# Notes:
#  To build and jemalloc
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
INSTALL_DIR="/usr/local/jemalloc"

####################################################################################
# Prepare for build
####################################################################################
if [ -f "${INSTALL_DIR}/lib/libjemalloc.a" ] ; then
    echo "jemalloc has been built, so do nothing"
    echo "Build jemalloc successfully"
    exit 0 
fi

$(tool_add_sudo) rm -fr ${BUILD_DIR}/${TARGET_DIR}*
mkdir -p ${BUILD_DIR}
tar -zxvf ${SERVER_FILENAME} -C ${BUILD_DIR}
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

echo "Finish build preparation......"
######################################################################################
# Build jemalloc
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/
./autogen.sh
./configure --prefix=${INSTALL_DIR}
make
make install

if [ ! -f /etc/ld.so.conf.d/jemalloc.conf ] ; then
    sudo echo "${INSTALL_DIR}/lib" >> /etc/ld.so.conf.d/jemalloc.conf
fi

ldconfig

popd > /dev/null

echo "Build jemalloc completed"

