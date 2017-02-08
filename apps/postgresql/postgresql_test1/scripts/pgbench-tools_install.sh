#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build PostgreSQL
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
INSTALL_DIR="/usr/local/postgresql"

#######################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/pgbench-tools/runset)"  == 0 ]; then
      echo "Pgbench-tools has been installed successfully"
      exit 0
fi

####################################################################################
# Prepare for install
####################################################################################
pushd ${INSTALL_DIR} > /dev/null
git clone https://github.com/gregs1104/pgbench-tools.git
popd > /dev/null

if [ "$(tool_check_exists ${INSTALL_DIR}/pgbench-tools/runset)"  == 0 ]; then
    echo "Install Pgbench-tools successfully"
else 
    echo "Fail to build Pgbench-tools"
    exit 1
fi
