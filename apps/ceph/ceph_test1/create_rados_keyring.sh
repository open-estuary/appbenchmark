#!/bin/bash

pushd "/etc/ceph" > /dev/null

if [ -f "ceph.client.radosgw.keyring" ] ; then
    echo "Ceph key ring has been created"
    exit 0
fi

sudo ceph-authtool  --create-keyring ceph.client.radosgw.keyring
sudo chmod +r ceph.client.radosgw.keyring

sudo ceph-authtool ceph.client.radosgw.keyring -n client.radosgw.gateway --gen-key
sudo ceph-authtool -n client.radosgw.gateway --cap osd 'all rwx' --cap mon 'allow rmx' ceph.client.radosgw.keyring
sudo ceph auth add client.radosgw.gateway -i ceph.client.radosgw.keyring

#scp radosgw.keyring to client gateway nodes
#
