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
if [ "$(tool_check_exists ${INSTALL_DIR}/bin/postgres)"  == 0 ]; then
      echo "PostgreSQL has been built successfully"
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
# Build PostgreSQL
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/
export USE_NAMED_POSIX_SEMAPHORES=1

CFLAGS_VALUE="-O3 -flto" 
#CFLAGS_VALUE="-O3 -flto -g -ggdb -fno-omit-frame-pointer" 

LIBS=-lpthread CFLAGS="${CFLAGS_VALUE}" ./configure --prefix=${INSTALL_DIR}
LIBS=-lpthread CFLAGS="${CFLAGS_VALUE}" make world -j 64
LIBS=-lpthread CFLAGS="${CFLAGS_VALUE}" make install-world

popd > /dev/null

##########################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/bin/postgres)"  == 0 ]; then
    echo "Build and install postgresql successfully"
else 
    echo "Fail to build postgresql"
    exit 1
fi
