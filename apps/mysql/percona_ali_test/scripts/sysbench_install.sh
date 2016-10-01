#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

source ~/.bashrc

######################################################################################
# Notes:
#  To build and sysbench
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

####################################################################################
# Prepare for build
####################################################################################
#if [ "$(which sysbench)" ] ; then
#    echo "sysbench has been built, so do nothing"
#    echo "Build sysbench successfully"
#    exit 0 
#fi

$(tool_add_sudo) rm -fr ${BUILD_DIR}/${TARGET_DIR}*
mkdir -p ${BUILD_DIR}
tar -zxvf ${SERVER_FILENAME} -C ${BUILD_DIR}
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

echo "Finish build preparation......"

######################################################################################
# Build sysbench
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/

CONFIGURE_OPTIONS=""
if [ $(uname -m) == "aarch64" ] ; then
    CONFIGURE_OPTIONS=${CONFIGURE_OPTIONS}" -buildtype=arm "
fi


./autogen.sh
./configure ${CONFIGURE_OPTIONS}
make
$(tool_add_sudo) make install

SYSBENCH_DB_DIR="~/apptests/sysbench/tests/db"
mkdir -p ${SYSBENCH_DB_DIR}
cp ./sysbench/tests/db/* ${SYSBENCH_DB_DIR}
cp ${APP_ROOT}/apps/mysql/percona_ali_test/config/*.lua ${SYSBENCH_DB_DIR}

echo "copy sysbench data to ${SYSBENCH_DB_DIR}"
popd > /dev/null

echo "Build sysbench completed"

