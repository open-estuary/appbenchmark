#!/bin/bash

#./setup.sh server

#Bind network interrupt to specific cpus
python ../../../toolset/perftools/miscs/set_ethirq_cpu_affinity.py 0 15

#Start 
start_cpu_num=20
inst_num=1
let "end_cpu_num=${start_cpu_num} + ${inst_num} - 1"

./scripts/start_server.sh ${start_cpu_num} ${end_cpu_num}
#./scripts/init_server.sh 17 18
