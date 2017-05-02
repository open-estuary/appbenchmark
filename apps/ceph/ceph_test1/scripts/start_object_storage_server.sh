#!/bin/bash

client_node=$1

BASE_DIR="/u01/ceph"

if [ -z "${client_node}" ] ; then
    echo "Must input client node ip"
    exit 1
fi

sudo ceph-deploy admin ${client_node}
sudo ceph-deploy rgw create
sudo ceph-deploy install --rgw ${client_node}



