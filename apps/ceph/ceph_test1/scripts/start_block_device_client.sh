#!/bin/bash

MONITOR_NODE=$1
IMAGE_NAME=$2
CLIENT_NODE=$3

if [ -z "${IMAGE_NAME}" ] ; then
    IMAGE_NAME="foo"
fi

BLOCK_DIR="/mnt/ceph-block-device"
BASE_DIR="/u01/ceph"

if [ -z "${MONITOR_NODE}" ] ; then
    echo "Must input monitor node name(or ip)"
    exit 1
fi

if [ -z "${CLIENT_NODE}" ] ; then
    CLIENT_NODE=$(hostname)
fi


if [ -d "${BLOCK_DIR}" ] ; then
    echo "${BLOCK_DIR} have been created "
    exit 0
fi

#Not necessary to execute both step1 and step2 if client node and montior node belong to the same node!

#Step1: Execute this command in client node
#sudo ceph-deploy install ${CLIENT_NODE}

#Step2: Execute this command in monitor node
#sudo ceph-deploy admin ${CLIENT_NODE}

#Step3: Create block device image
sudo chmod +r /etc/ceph/ceph.client.admin.keyring
sudo rbd create ${IMAGE_NAME} --size 4096 -m  ${MONITOR_NODE}  

#Step4: Map the image to a block device
sudo rbd map ${IMAGE_NAME} --name client.admin -m ${MONITOR_NODE} 

#Step5: create file system
sudo mkfs.xfs /dev/rbd/rbd/${IMAGE_NAME}

#sudo mkfs.xfs -m0 /dev/rbd/rbd/foo
#Step6: Mount the file system on ceph-client node
sudo mkdir -p "${BLOCK_DIR}"
sudo mount /dev/rbd/rbd/foo "${BLOCK_DIR}"


