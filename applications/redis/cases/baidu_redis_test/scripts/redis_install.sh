#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build and install Redis-Server 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/src/redis-server)"  != 0 ]; then
    echo "Redis server has not been built yet"
    exit 1
fi

if [ "$(ps -aux | grep "redis-server" | grep -v "grep")" != "" ]; then
    echo "Redis server is running"
    exit 0
fi

###########################################################################################
# Begin to install Redis-server
###########################################################################################
#Step1:Setup redis-server
echo "Begin to install redis server ......"
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/
REDIS_INSTALL_DIR="/usr/local/redis/"

cd src
sudo make PREFIX=${REDIS_INSTALL_DIR} install

#Install step 2: Prepare redis config
sudo mkdir ${REDIS_INSTALL_DIR}/config
sudo cp ${APP_ROOT}/applications/redis/cases/baidu_redis_test/config/redis*.conf ${REDIS_INSTALL_DIR}/config/

echo 1 > /proc/sys/net/ipv4/tcp_timestamps

if [ "$(tool_check_exists ${REDIS_INSTALL_DIR}/config/redis_cpu0_port7000.conf)" == 0 ] ; then
    echo "Redis server has been installed successfully"
else
    echo "Fail to install Redis server, please check it manually"
fi 
popd > /dev/null

