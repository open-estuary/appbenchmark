#!/bin/bash

default_ip="192.168.10.174"
ip=${1}

if [ $# -lt 1 ] ; then 
    echo "Usage: ./run_test.sh {ip} ${test_multi_num=1}"
    ip=${default_ip}
    echo "Use default IP:${ip}"
fi

./run_client.sh ${ip} 

#./scripts/warmup.sh ${ip}

index=1
while [[ ${index} -lt 20 ]] 
do
./scripts/init_client.sh ${ip} test ${index}
sleep 7200
let "index++"
done

