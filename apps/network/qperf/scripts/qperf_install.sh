#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build Lmbench 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

if [ -z "$(which qperf 2>/dev/null)" ] ; then
    echo "Fail to use yum or apt-get to install qperf"
    exit 1
fi

##########################################################################################
