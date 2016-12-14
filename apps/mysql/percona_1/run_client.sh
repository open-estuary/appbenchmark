#!/bin/bash

default_ip="192.168.10.174"

ip=${1}

if [ $# -lt 1 ] ; then 
    echo "Usage: ./run_client.sh {ip}"
    exit 0
fi

./scripts/init_client.sh ${ip} init
./scripts/init_client.sh ${ip} loaddata

