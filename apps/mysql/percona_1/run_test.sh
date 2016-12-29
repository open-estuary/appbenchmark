#!/bin/bash

if [ -z "${1}" ] ; then 
    echo "Usage: ./run_test.sh {percona_server_ip}"
    exit 0
fi

ip=${1}

echo 1 > /proc/sys/kernel/numa_balancing

service irqbalance stop
#Bind network interrupt to specific cpus
python ../../../toolset/perftools/miscs/set_ethirq_cpu_affinity.py 0 7

./run_client.sh ${ip} 

#./scripts/warmup.sh ${ip}

index=1
while [[ ${index} -lt 6 ]] 
do
./scripts/init_client.sh ${ip} test ${index}
ps -aux | grep sysbench | grep -v grep | awk '{print $2}' | xargs -n 1 taskset -pc 8-63
ps -aux | grep run | grep -v grep | awk '{print $2}' | xargs -n 1 taskset -pc 8-63
sleep 3600
let "index++"
done

