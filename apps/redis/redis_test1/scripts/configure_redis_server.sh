#!/bin/bash

#Define global APP_ROOT directory
if [ -z "${APP_ROOT}" ]; then
    # Default value
    APP_ROOT=$(cd `dirname $0` ; cd ../../../; pwd)
else
    # Re-declare so it can be used in this script
    APP_ROOT=$(echo $APP_ROOT)
fi

#######################################################################################
if [ "$(ps -aux | grep "redis-server" | grep -v "grep")" != "" ]; then
    echo "Redis server is running"
    exit 0
fi

###########################################################################################
# Begin to configure Redis-server
###########################################################################################

echo "Begin to configure redis server ......"
cd ${TARGET_DIR}/
REDIS_INSTALL_DIR="/usr/local/redis/"

sudo mkdir -p ${REDIS_INSTALL_DIR}/config
sudo cp ${APP_ROOT}/redis/redis_test1/config/redis*.conf ${REDIS_INSTALL_DIR}/config/

sudo echo 1 > /proc/sys/net/ipv4/tcp_timestamps
sudo echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse
sudo echo 1 > /proc/sys/net/ipv4/tcp_tw_recycle
sudo echo 2048 65000 > /proc/sys/net/ipv4/ip_local_port_range
sudo echo 2621440 > /proc/sys/net/core/somaxconn
sudo echo 2621440 > /proc/sys/net/core/netdev_max_backlog
sudo echo 2621440 > /proc/sys/net/ipv4/tcp_max_syn_backlog
#sudo echo 1 > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_time_wait
#sudo echo 2621440 > /proc/sys/net/netfilter/nf_conntrack_max

#support maxinum number of files open
#sudo ulimit -n 102400

if [ -d  "/writeable-proc/" ]; then
    echo "Provision configurations on docker iamge"

    sudo echo 1 > /writeable-proc/sys/net/ipv4/tcp_tw_reuse
    sudo echo 1 > /writeable-proc/sys/net/ipv4/tcp_tw_recycle
    sudo echo 2048 65000 > /writeable-proc/sys/net/ipv4/ip_local_port_range
    sudo echo 2621440 > /writeable-proc/sys/net/core/somaxconn
    sudo echo 2621440 > /writeable-proc/sys/net/core/netdev_max_backlog
    sudo echo 2621440 > /writeable-proc/sys/net/ipv4/tcp_max_syn_backlog

    sudo echo 1 > /writeable-proc/sys/net/netfilter/nf_conntrack_tcp_timeout_time_wait
    sudo echo 150 > /writeable-proc/sys/net/netfilter/nf_conntrack_tcp_timeout_established
    sudo echo 60 > /writeable-proc/sys/net/netfilter/nf_conntrack_tcp_timeout_close_wait
    sudo echo 120 > /writeable-proc/sys/net/netfilter/nf_conntrack_tcp_timeout_fin_wait
    sudo echo 1 > /writeable-proc/sys/net/netfilter/nf_conntrack_tcp_timeout_time_wait
    sudo echo 2621440 > /writeable-proc/sys/net/netfilter/nf_conntrack_max
fi


if [ -f "${REDIS_INSTALL_DIR}/config/redis_cpu0_port7000.conf" ] ; then
    echo "Redis server has been installed successfully"
else
    echo "Fail to install Redis server, please check it manually"
fi 

