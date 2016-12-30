#!/bin/bash

if [ -z "${1}" ] ; then
    echo "Usage: ./run_client.sh <server ip>"
    exit 0
fi

ip=${1}
#Notes: Userid and passwd have been specified in scripts/init_client.sh

./setup.sh client
