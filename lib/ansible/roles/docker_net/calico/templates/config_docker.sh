#!/bin/bash

echo "{" > /etc/docker/daemon.json
echo -e "  \"cluster-store\":\"etcd://{{ Etcd_Server }}:2379\" " >> /etc/docker/daemon.json
echo "}" >> /etc/docker/daemon.json

systemctl restart docker.service

