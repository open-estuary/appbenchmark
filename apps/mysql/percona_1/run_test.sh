#!/bin/bash

default_ip="192.168.10.174"
ip=${1}

if [ $# -lt 1 ] ; then 
    echo "Usage: ./run_test.sh {ip}"
    ip=${default_ip}
    echo "Use default IP:${ip}"
fi

./run_client.sh ${ip} 

#./scripts/warmup.sh ${ip}
./scripts/init_client.sh ${ip} test
