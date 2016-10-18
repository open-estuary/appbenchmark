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
$(tool_add_sudo) make PREFIX=${REDIS_INSTALL_DIR} install

#Install step 2: Prepare redis config
$(tool_add_sudo) mkdir ${REDIS_INSTALL_DIR}/config
$(tool_add_sudo) cp ${APP_ROOT}/apps/redis/redis_test1/config/redis*.conf ${REDIS_INSTALL_DIR}/config/

$(tool_add_sudo) echo 1 > /proc/sys/net/ipv4/tcp_timestamps
$(tool_add_sudo) echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse
$(tool_add_sudo) echo 1 > /proc/sys/net/ipv4/tcp_tw_recycle
$(tool_add_sudo) echo 2048 65000 > /proc/sys/net/ipv4/ip_local_port_range
$(tool_add_sudo) echo 2621440 > /proc/sys/net/core/somaxconn
$(tool_add_sudo) echo 2621440 > /proc/sys/net/core/netdev_max_backlog
$(tool_add_sudo) echo 2621440 > /proc/sys/net/ipv4/tcp_max_syn_backlog
$(tool_add_sudo) echo 1 > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_time_wait
$(tool_add_sudo) echo 2621440 > /proc/sys/net/netfilter/nf_conntrack_max

#support maxinum number of files open
$(tool_add_sudo) ulimit -n 102400

if [ "$(tool_check_exists /writeable-proc)" == 0 ]; then
    echo "Provision configurations on docker iamge"

    $(tool_add_sudo) echo 1 > /writeable-proc/sys/net/ipv4/tcp_tw_reuse
    $(tool_add_sudo) echo 1 > /writeable-proc/sys/net/ipv4/tcp_tw_recycle
    $(tool_add_sudo) echo 2048 65000 > /writeable-proc/sys/net/ipv4/ip_local_port_range
    $(tool_add_sudo) echo 2621440 > /writeable-proc/sys/net/core/somaxconn
    $(tool_add_sudo) echo 2621440 > /writeable-proc/sys/net/core/netdev_max_backlog
    $(tool_add_sudo) echo 2621440 > /writeable-proc/sys/net/ipv4/tcp_max_syn_backlog

    $(tool_add_sudo) echo 1 > /writeable-proc/sys/net/netfilter/nf_conntrack_tcp_timeout_time_wait
    $(tool_add_sudo) echo 150 > /writeable-proc/sys/net/netfilter/nf_conntrack_tcp_timeout_established
    $(tool_add_sudo) echo 60 > /writeable-proc/sys/net/netfilter/nf_conntrack_tcp_timeout_close_wait
    $(tool_add_sudo) echo 120 > /writeable-proc/sys/net/netfilter/nf_conntrack_tcp_timeout_fin_wait
    $(tool_add_sudo) echo 1 > /writeable-proc/sys/net/netfilter/nf_conntrack_tcp_timeout_time_wait
    $(tool_add_sudo) echo 2621440 > /writeable-proc/sys/net/netfilter/nf_conntrack_max
fi

if [ "$(tool_check_exists ${REDIS_INSTALL_DIR}/config/redis_cpu0_port7000.conf)" == 0 ] ; then
    echo "Redis server has been installed successfully"
else
    echo "Fail to install Redis server, please check it manually"
fi 
popd > /dev/null

