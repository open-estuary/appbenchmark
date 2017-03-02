#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build and install benchmark tools including scripts 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

INSTALL_DIR="/usr/local/mariadb"
#######################################################################################
pushd ${INSTALL_DIR} > /dev/null

bzr branch lp:mariadb-tools

popd > /dev/null
##########################################################################################
echo "Download benchmark tools successfully"
