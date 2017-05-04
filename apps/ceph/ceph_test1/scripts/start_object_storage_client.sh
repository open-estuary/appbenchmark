#!/bin/bash

client_node=$1

BASE_DIR="/u01/ceph"

if [ -z "${client_node}" ] ; then
    echo "Must input client node ip"
    exit 1
fi

pushd ${BASE_DIR} > /dev/null
sudo ceph-deploy admin ${client_node}
sudo ceph-deploy rgw create ${client_node}
sudo ceph-deploy install --rgw ${client_node}
popd > /dev/null
