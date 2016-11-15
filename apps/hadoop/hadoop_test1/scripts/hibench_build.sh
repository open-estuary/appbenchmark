#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build HiBench
#
#####################################################################################

BUILD_DIR="./build_hibench"
CTAG_NAME="6.0"
BRANCH_NAME="hibench"
TARGET_DIR=${BRANCH_NAME}${CTAG_NAME}

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR})"  == 0 ]; then
      echo "Hibench has been built successfully"
      exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
$(tool_add_sudo) rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR} > /dev/null

git clone https://github.com/intel-hadoop/HiBench.git ${TARGET_DIR}
cd ./${TARGET_DIR}
git checkout ${CTAG_NAME}

######################################################################################
# Build HiBench
#####################################################################################
mvn -Phadoopbench clean package

