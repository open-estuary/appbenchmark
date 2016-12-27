#!/bin/bash

echo 1 > /proc/sys/kernel/numa_balancing

if [ -z "${1}" ] ; then
    ip="127.0.0.1"
else 
    ip=${1}
fi

./scripts/init_client.sh ${ip} test 200   > ./testresult_200_1thread
./scripts/init_client.sh ${ip} test 200 4 > ./testresult_200_4thread
./scripts/init_client.sh ${ip} test 200 8 > ./testresult_200_8thread

