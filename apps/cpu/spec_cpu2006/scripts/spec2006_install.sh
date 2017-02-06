#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build SpecCPU2006
#
#####################################################################################

SPEC_CPU2006FILENAME="specjvm2006.tar.gz"
if [ ! -f "./"${SPEC_CPU2006FILENAME} ] ; then
    echo "Please install SPEC_CPU2006:${SPEC_CPU2006FILENAME} firstly due to license issue"
    exit 0
fi 

BUILD_DIR="./"$(tool_get_build_dir ${SPEC_CPU2006FILENAME})
SERVER_FILENAME=$SPEC_CPU2006FILENAME
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
INSTALL_DIR="/usr/local/bin"

#######################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/runspec)"  == 0 ]; then
      echo "SpecCPU2006 has been installed successfully"
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
# Build lmbench
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR}/${TARGET_DIR} > /dev/null
if [ ! -f "./buildtools" ] ; then
    cd speccpu2006
fi

export FORCE_UNSAFE_CONFIGURE=1
echo y | ./buildtools


echo "**********************************************************************************"
popd > /dev/null

##########################################################################################
echo "Build SpecCPU2006 successfully"
