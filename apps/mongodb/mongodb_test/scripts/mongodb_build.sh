#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build MongoDB
#
#####################################################################################

#BUILD_DIR="./"$(tool_get_build_dir $1)
BUILD_DIR="./builddir_mongodb"
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
INSTALL_DIR="/usr/local/mongodb"
TAG_VERSION="r3.4.0"

source /etc/profile
export PATH=/usr/local/bin:$PATH

####################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/bin/mongod)"  == 0 ]; then
    echo "MongoDB has been built successfully"
    exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
#$(tool_add_sudo) rm -fr ${BUILD_DIR}
if [ ! -d ${BUILD_DIR} ] ; then
    mkdir ${BUILD_DIR}
fi

#tar -zxvf ${SERVER_FILENAME} -C ${BUILD_DIR}
#TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

echo "Finish build preparation......"

######################################################################################
# Build MongoDB
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/null
git clone https://github.com/mongodb/mongo.git
TARGET_DIR="mongo"

cd ${TARGET_DIR}/
git pull
git checkout ${TAG_VERSION}

#CFLAGS_VALUE="-O3 -flto" 
#CFLAGS_VALUE="-O3 -flto -g -ggdb -fno-omit-frame-pointer" 
#CFLAGS_VALUE="-O3  -fno-omit-frame-pointer" 

scons core -j 32 CCFLAGS='-march=armv8-a+crc -mtune=generic' --release
scons install --prefix=${INSTALL_DIR} -j 32 CCFLAGS='-march=armv8-a+crc -mtune=generic' --release

if [ -z "$(grep ${INSTALL_DIR} /etc/profile)" ] ; then
    sudo sed -i '$ a export PATH=$PATH:/usr/local/mongodb/bin' /etc/profile
fi

popd > /dev/null

##########################################################################################
if [ "$(tool_check_exists ${INSTALL_DIR}/bin/mongod)"  == 0 ]; then
    echo "Build and install mongodb successfully"
else 
    echo "Fail to build mongodb"
    exit 1
fi
