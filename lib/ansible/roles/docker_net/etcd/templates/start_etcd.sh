#!/bin/bash

#echo "export ETCD_UNSUPPORTED_ARCH=arm64" >> /etc/profile
#source /etc/profile
export ETCD_UNSUPPORTED_ARCH=arm64
(etcd --listen-client-urls "http://{{ item }}:2379" --advertise-client-urls "http://{{ item }}:2379" &)
