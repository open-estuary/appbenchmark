#!/bin/bash

#echo "export ETCD_UNSUPPORTED_ARCH=arm64" >> /etc/profile
#source /etc/profile
if [ {{ Arch }} = "arm64" ]; then
	export ETCD_UNSUPPORTED_ARCH=arm64
fi
(etcd --listen-client-urls "http://{{ etcd_ip }}:2379" --advertise-client-urls "http://{{ etcd_ip }}:2379" &)
