#!/bin/bash

#ip="192.168.1.187"

ip=${1}

if [ $# -lt 1 ] ; then 
    echo "Usage: ./run_client.sh {ip}"
    exit 0
fi

./scripts/init_client.sh init
./scripts/init_client.sh loaddata

