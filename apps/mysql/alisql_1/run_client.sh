#!/bin/bash

if [ -z "${1}" ] ; then
    echo "Usage: ./run_client.sh <server ip (not local ip address)>"
    exit 0  
fi
ip=${1}

#Notes: Userid and passwd have been specified in scripts/init_client.sh

./setup.sh client

#Start to initialize 200 mysql instances
./scripts/init_client.sh ${ip} init 200
