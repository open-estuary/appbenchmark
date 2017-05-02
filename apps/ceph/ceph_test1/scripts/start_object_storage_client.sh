#!/bin/bash

monitor_node=$1
client_node=$2

BASE_DIR="/u01/ceph"

if [ -z "${monitor_node}" ] ; then
    echo "Must input monitor node ip"
    exit 1
fi

if [ -z "${client_node}" ] ; then
    client_node=$(hostname)
fi

#Not necessary to execute both step1 and step2 if client node and montior node belong to the same node!

#Step1: Execute this command in client node
sudo ceph-deploy install ${client_node}




