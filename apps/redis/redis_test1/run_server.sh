#!/bin/bash

#./setup.sh server

#Start 
start_cpu_num=17
inst_num=10
let "end_cpu_num=${start_cpu_num} + ${inst_num} - 1"

echo "Stop irqbalance firstly"
service irqbalance stop

echo "Disable CPU 32~64"
./scripts/enable_cpus.sh 32 64 0

#Bind network interrupt to specific cpus
python ../../../toolset/perftools/miscs/set_ethirq_cpu_affinity.py 0 15
./scripts/start_server.sh ${start_cpu_num} ${end_cpu_num}
#./scripts/init_server.sh 17 18
