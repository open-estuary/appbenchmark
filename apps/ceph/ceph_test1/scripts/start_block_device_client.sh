#!/bin/bash

monitor_node=$1
client_node=$2

BASE_DIR="/u01/ceph"

if [ -z "${monitor_node}" ] ; then
    echo "Must input monitor node name(or ip)"
    exit 1
fi

if [ -z "${client_node}" ] ; then
    client_node=$(hostname)
fi

#Not necessary to execute both step1 and step2 if client node and montior node belong to the same node!

#Step1: Execute this command in client node
#sudo ceph-deploy install ${client_node}

#Step2: Execute this command in monitor node
#sudo ceph-deploy admin ${client_node}

#Step3: Create block device image
sudo chmod +r /etc/ceph/ceph.client.admin.keyring
sudo rbd create foo --size 4096 -m  ${monitor_node}  

#Step4: Map the image to a block device
sudo rbd map foo --name client.amdin -m ${monitor_node} 

#Step5: create file system
sudo mkfs.xfs -m0 /dev/rbd/rbd/foo

#Step6: Mount the file system on ceph-client node
sudo mkdir /mnt/ceph-block-device
sudo mount /dev/rbd/rbd/foo /mnt/ceph-block-device
cd /mnt/ceph-block-device

