#!/bin/bash

if [ $# -lt 1 ] ; then 
    echo "Usage: ./run_client.sh {ip}"
    exit 0
fi

ip=${1}

./setup.sh client

./scripts/init_client.sh ${ip} init
./scripts/init_client.sh ${ip} loaddata

