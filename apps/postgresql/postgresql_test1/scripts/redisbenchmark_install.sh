#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build and install PostgreSQL-benchmark 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/src/postgresql-benchmark)"  != 0 ]; then
    echo "PostgreSQL server has not been built yet"
    exit 1
fi

###########################################################################################
# Begin to install PostgreSQL-benchmark
###########################################################################################
#Step1:Setup postgresql-server
echo "Begin to install postgresql benchmark ......"
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/
REDIS_INSTALL_DIR="/usr/local/postgresql/"

cd src
$(tool_add_sudo) make PREFIX=${REDIS_INSTALL_DIR} install


$(tool_add_sudo) echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse
$(tool_add_sudo) echo 1 > /proc/sys/net/ipv4/tcp_tw_recycle
$(tool_add_sudo) echo 2048 65000 > /proc/sys/net/ipv4/ip_local_port_range
$(tool_add_sudo) echo 262144 > /proc/sys/net/core/somaxconn
$(tool_add_sudo) echo 262144 > /proc/sys/net/core/netdev_max_backlog
$(tool_add_sudo) echo 262144 > /proc/sys/net/ipv4/tcp_max_syn_backlog
$(tool_add_sudo) echo 1 > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_time_wait
$(tool_add_sudo) echo 262144 > /proc/sys/net/netfilter/nf_conntrack_max

#support maxinum number of files open
$(tool_add_sudo) ulimit -n 102400

if [ "$(tool_check_exists /writeable-proc)" == 0 ]; then
    echo "Provision configurations on docker iamge"

    $(tool_add_sudo) echo 1 > /writeable-proc/sys/net/ipv4/tcp_tw_reuse
    $(tool_add_sudo) echo 1 > /writeable-proc/sys/net/ipv4/tcp_tw_recycle
    $(tool_add_sudo) echo 2048 65000 > /writeable-proc/sys/net/ipv4/ip_local_port_range
    $(tool_add_sudo) echo 262144 > /writeable-proc/sys/net/core/somaxconn
    $(tool_add_sudo) echo 262144 > /writeable-proc/sys/net/core/netdev_max_backlog
    $(tool_add_sudo) echo 262144 > /writeable-proc/sys/net/ipv4/tcp_max_syn_backlog

fi

echo "PostgreSQL-benchmark has been installed successfully"

popd > /dev/null

