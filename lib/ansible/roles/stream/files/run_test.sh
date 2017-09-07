#!/bin/bash

if [ -z "$(which stream 2>/dev/null)" ] ; then
    echo "Please install stream firstly !"
    exit 0
fi

#filepath=$(cd "$(dirname "$0")"; pwd);
#FILENAME="$filepath/hosts"

#i=0
#while read line
# do
#   IP_ARRAY[$i]=$line
#   i=$[$i+1]
# done  < $FILENAME

IP_ARR=`ip addr | grep "eth0" | grep "inet" | awk '{ print $2}'`
IP=${IP_ARR%/*}

CPUCORES=`lscpu | grep "^CPU(s):" | awk '{$2 = $2 + 0; print $2}'`
#CUR_DIR=$(cd `dirname $0`; pwd)
#${CUR_DIR}/stream_test.sh ${CPUCORES}

#The following commands are just listed here for references 
#exit 0

function stream_test_64core() {
       nohup echo ": streamv2core0-63" >> ~/result_memory/stream/result_${IP} 2>&1
       nohup numactl --cpunodebind=0,1,2,3 --localalloc stream -v 1 -M 200M -P ${CPUCORES} >> ~/result_memory/stream/result_${IP} 2>&1
                sleep 60
       nohup numactl --cpunodebind=0,1,2,3 --localalloc stream -v 2 -M 200M -P ${CPUCORES} >> ~/result_memory/stream/result_${IP} 2>&1
                sleep 60
}

for ((i=0;i<3;i++));do
        stream_test_64core
done


#Basic commands
#stream -P threads_num -M data_size -W warmup -N repetion_times

:<<'
nohup echo "Test V1 mode ..." >> ~/result_memory/stream/result_${IP} 2>&1
nohup numactl  --localalloc stream -v 1 -M 200M -P 64 -N 10 >> ~/result_memory/stream/result_${IP} 2>&1

nohup echo "Test V2 mode ..." >> ~/result_memory/stream/result_${IP} 2>&1
nohup numactl  --localalloc stream -v 2 -M 200M -P 64 -N 10 >> ~/result_memory/stream/result_${IP} 2>&1

nohup taskset -c 0   stream -v 2 -M 100M -P 1 >> ~/result_memory/stream/result_${IP} 2>&1
nohup taskset -c 0-3 stream -v 2 -M 100M -P 4 >> ~/result_memory/stream/result_${IP} 2>&1

nohup numactl --cpunodebind=0       --membind=0,1,2,3 stream -v 2 -M 100M -P 16 >> ~/result_memory/stream/result_${IP} 2>&1
nohup numactl --cpunodebind=0,1     --membind=0,1,2,3 stream -v 2 -M 100M -P 32 >> ~/result_memory/stream/result_${IP} 2>&1
nohup numactl --cpunodebind=2,3     --membind=0,1,2,3 stream -v 2 -M 100M -P 32 >> ~/result_memory/stream/result_${IP} 2>&1
nohup numactl --cpunodebind=0,1,2,3 --membind=0,1,2,3 stream -v 2 -M 100M -P 64 >> ~/result_memory/stream/result_${IP} 2>&1

#echo ""
#echo "Test V1 Mode..."

nohup taskset -c 0   stream -v 1 -M 32M -P 1 >> ~/result_memory/stream/result_${IP} 2>&1
nohup taskset -c 0-3 stream -v 1 -M 32M -P 4 >> ~/result_memory/stream/result_${IP} 2>&1

nohup numactl --cpunodebind=0       --membind=0,1,2,3 stream -v 1 -M 32M -P 16 >> ~/result_memory/stream/result_${IP} 2>&1
nohup numactl --cpunodebind=0,1     --membind=0,1,2,3 stream -v 1 -M 32M -P 32 >> ~/result_memory/stream/result_${IP} 2>&1
nohup numactl --cpunodebind=2,3     --membind=0,1,2,3 stream -v 1 -M 32M -P 32 >> ~/result_memory/stream/result_${IP} 2>&1
nohup numactl --cpunodebind=0,1,2,3 --membind=0,1,2,3 stream -v 1 -M 32M -P 64 >> ~/result_memory/stream/result_${IP} 2>&1
'
