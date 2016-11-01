#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build and install Percona-Server 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
SUBBUILD_DIR="armbuild"

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/${SUBBUILD_DIR}/sql/mysqld)"  != 0 ]; then
    echo "Percona server has not been built yet"
    exit 1
fi

if [ "$(ps -aux | grep "/u01/my3306/bin/mysqld_safe" | grep -v "grep")" != "" ]; then
    echo "Percona server is running"
    exit 0
fi

###########################################################################################
# Begin to install percona
###########################################################################################
#Install step 1: Install percona server
echo "Begin to install percona server ......"
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/${SUBBUILD_DIR}

$(tool_add_sudo) make install

echo "Start to start Percona Server ......"
./start_server.sh

echo "Pecora server build and install complete"
popd > /dev/null

