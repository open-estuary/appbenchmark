#!/bin/bash

if [ -z "${1}" ] ; then
    echo "Usage: ./run_client.sh <server ip (not local ip address)>"
    exit 0  
fi

ip=${1}

echo 1 > /proc/sys/kernel/numa_balancing
ulimit -n 10240

./scripts/init_client.sh ${ip} test 200   > ./testresult_200_1thread
./scripts/init_client.sh ${ip} test 200 4 > ./testresult_200_4thread
./scripts/init_client.sh ${ip} test 200 8 > ./testresult_200_8thread

