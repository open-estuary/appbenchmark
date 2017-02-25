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
INSTALL_DIR="/usr/local/pgbouncer"

####################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/bin/pgbouncer)"  == 0 ]; then
    echo "PostgreSQL has been installed successfully"
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

#CFLAGS_VALUE="-O3 -flto" 
#CFLAGS_VALUE="-O3 -flto -g -ggdb -fno-omit-frame-pointer" 
#CFLAGS_VALUE="-O3  -fno-omit-frame-pointer" 

./configure --prefix=${INSTALL_DIR}
make -j 64
make install

if [ -z "$(grep ${INSTALL_DIR} /etc/profile)" ] ; then
    sudo sed -i '$ a export PATH=$PATH:/usr/local/pgbouncer/bin' /etc/profile
fi

popd > /dev/null

##########################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/bin/pgbouncer)"  == 0 ]; then
    echo "Build and install pgbouncer successfully"
else 
    echo "Fail to build pgbouncer"
    exit 1
fi
