#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
${CUR_DIR}/setup.sh server

INTEFACE=${1}
IP=$2

if [ -z "${IP}" ] ; then
    echo "Usage: ./run_server.sh <local network interface> <local IP address>"
    exit 0
fi

echo "Initialize server side"
ifconfig ${INTERFACE} ${IP}
ethtool -C ${INTERFACE} rx-usecs 30 rx-frames 30 tx-usecs 30 tx-frames 30
service iptables stop
service network-manager stop


echo "Start Server Right Now"
qperf 


