#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build and install AliSQL-Server 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
SUBBUILD_DIR="armbuild"

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/${SUBBUILD_DIR}/sql/mysqld)"  != 0 ]; then
    echo "AliSQL server has not been built yet"
    exit 1
fi

if [ "$(ps -aux | grep "/u01/my3306/bin/mysqld_safe" | grep -v "grep")" != "" ]; then
    echo "AliSQL server is running"
    exit 0
fi

###########################################################################################
# Begin to install alisql
###########################################################################################
#Install step 1: Install alisql server
echo "Begin to install alisql server ......"
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/${SUBBUILD_DIR}

pwd

$(tool_add_sudo) make install

#Install step 2: Add 'mysql' test user account and rights
$(tool_add_sudo) groupadd mysql
$(tool_add_sudo) useradd -g mysql mysql

echo "Alisql server has been installed"
popd > /dev/null

