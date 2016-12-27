#!/bin/bash

default_ip="192.168.11.174"
ip=${1}

if [ $# -lt 1 ] ; then 
    echo "Usage: ./run_test.sh {ip} ${test_multi_num=1}"
    ip=${default_ip}
    echo "Use default IP:${ip}"
fi

echo 1 > /proc/sys/kernel/numa_balancing

service irqbalance stop
#Bind network interrupt to specific cpus
python ../../../toolset/perftools/miscs/set_ethirq_cpu_affinity.py 0 7

./run_client.sh ${ip} 

#./scripts/warmup.sh ${ip}

index=1
while [[ ${index} -lt 20 ]] 
do
./scripts/init_client.sh ${ip} test ${index}
ps -aux | grep sysbench | grep -v grep | awk '{print $2}' | xargs -n 1 taskset -pc 8-63
ps -aux | grep run | grep -v grep | awk '{print $2}' | xargs -n 1 taskset -pc 8-63
sleep 3600
let "index++"
done

