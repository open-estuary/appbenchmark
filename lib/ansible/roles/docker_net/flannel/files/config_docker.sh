#!/bin/bash

FLANNEL_SUBNET=`cat /run/flannel/subnet.env | grep FLANNEL_SUBNET | awk -F '=' '{print $2}'`
FLANNEL_MTU=`cat /run/flannel/subnet.env | grep FLANNEL_MTU | awk -F '=' '{print $2}'`

echo "{" > /etc/docker/daemon.json
echo -e "  \"bip\":\"$FLANNEL_SUBNET\",\n  \"mtu\":$FLANNEL_MTU " >> /etc/docker/daemon.json
echo "}" >> /etc/docker/daemon.json

systemctl restart docker.service
