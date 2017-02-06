#!/bin/bash

if [ -z "$(which stream 2>/dev/null)" ] ; then
    echo "Please install stream firstly !"
    exit 0
fi


#Basic commands
#stream -P threads_num -M data_size -W warmup -N repetion_times

echo "Test V1 mode ..."
numactl  --localalloc stream -v 1 -M 200M -P 64 -N 10

echo "Test V2 mode ..."
numactl  --localalloc stream -v 2 -M 200M -P 64 -N 10

#The following commands are just listed here for references 
exit 0

taskset -c 0   stream -v 2 -M 100M -P 1
taskset -c 0-3 stream -v 2 -M 100M -P 4

numactl --cpunodebind=0       --membind=0,1,2,3 stream -v 2 -M 100M -P 16
numactl --cpunodebind=0,1     --membind=0,1,2,3 stream -v 2 -M 100M -P 32
numactl --cpunodebind=2,3     --membind=0,1,2,3 stream -v 2 -M 100M -P 32
numactl --cpunodebind=0,1,2,3 --membind=0,1,2,3 stream -v 2 -M 100M -P 64

echo ""
echo "Test V1 Mode..."

taskset -c 0   stream -v 1 -M 32M -P 1
taskset -c 0-3 stream -v 1 -M 32M -P 4

numactl --cpunodebind=0       --membind=0,1,2,3 stream -v 1 -M 32M -P 16
numactl --cpunodebind=0,1     --membind=0,1,2,3 stream -v 1 -M 32M -P 32
numactl --cpunodebind=2,3     --membind=0,1,2,3 stream -v 1 -M 32M -P 32
numactl --cpunodebind=0,1,2,3 --membind=0,1,2,3 stream -v 1 -M 32M -P 64

