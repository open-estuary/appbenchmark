#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build and install Redis-benchmark 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/src/redis-benchmark)"  != 0 ]; then
    echo "Redis server has not been built yet"
    exit 1
fi

###########################################################################################
# Begin to install Redis-benchmark
###########################################################################################
#Step1:Setup redis-server
echo "Begin to install redis benchmark ......"
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/
REDIS_INSTALL_DIR="/usr/local/redis/"

cd src
sudo make PREFIX=${REDIS_INSTALL_DIR} install


echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse
echo 1 > /proc/sys/net/ipv4/tcp_tw_recycle
echo 2048 65000 > /proc/sys/net/ipv4/ip_local_port_range
echo 262144 > /proc/sys/net/core/somaxconn
echo 262144 > /proc/sys/net/core/netdev_max_backlog
echo 262144 > /proc/sys/net/ipv4/tcp_max_syn_backlog

echo "Redis-benchmark has been installed successfully"
popd > /dev/null

