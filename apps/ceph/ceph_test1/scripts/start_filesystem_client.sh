#!/bin/bash

monitor_node=$1
client_node=$2

BASE_DIR="/u01/ceph"

fs_pool_name="cephfs_data"
metadata_pool_name="cephfs_metadata"
filename="ceph_testfs"
pg_num=64

if [ -z "${monitor_node}" ] ; then
    echo "Must input monitor node ip"
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

#Step3: Create A Filesystem
sudo ceph osd pool create ${fs_pool_name} ${pg_num}
sudo ceph osd pool create ${metadata_pool_name} ${pg_num}

#Just for test
sudo ceph osd pool set ${fs_pool_name} min_size ${min_replica_num}
sudo ceph osd pool set ${metadata_pool_name} min_size ${min_replica_num}
#Create fs
sudo ceph fs new ${filename} ${metadata_pool_name} ${fs_pool_name}
sudo ceph fs ls

#Step4: Map the image to a block device
#sudo mkdir /mnt/mycephfs
#sudo mount -t ceph ${monitor_node}:6789:/ /mnt/mycephfs

#or mount Ceph FS as a Filesystem in User Space(FUSE)
sudo ${INSTALL_CMD} install -y ceph-fuse
sudo mkdir ~/mycephfs
sudo ceph-fuse -m ${monitor_node}:6789 ~/mycephfs

