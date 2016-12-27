#!/bin/bash

if [ -z "${1}" ] ; then
    ip="127.0.0.1"
else 
    ip=${1}
fi

#Notes: Userid and passwd have been specified in scripts/init_client.sh

./setup.sh client

#Start to initialize 200 mysql instances
./scripts/init_client.sh ${ip} init 200
