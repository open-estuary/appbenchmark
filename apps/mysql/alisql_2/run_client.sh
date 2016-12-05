#!/bin/bash

ip=${1}
#Notes: Userid and passwd have been specified in scripts/init_client.sh

./setup.sh client

#Start to initialize 6 mysql instances
./scripts/init_client.sh ${ip} init 6
