#!/bin/bash

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

MAX_CPUCORES=$1
function stream_test() {
    nohup echo ": streamv1 core0" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl -C 0 --localalloc stream -v 1 -M 200M -P 1 >> ~/result_memory/stream/result_${IP} 2>&1

    nohup echo ": streamv2 core0" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl -C 0 --localalloc stream -v 2 -M 200M -P 1 >> ~/result_memory/stream/result_${IP} 2>&1

    nohup echo ": streamv1 core0-3" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl -C 0-3 --localalloc stream -v 1 -M 200M -P 4 >> ~/result_memory/stream/result_${IP} 2>&1
    
    nohup echo ": streamv2 core0-3" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl -C 0-3 --localalloc stream -v 2 -M 200M -P 4 >> ~/result_memory/stream/result_${IP} 2>&1

    nohup echo ": streamv1 core0-15" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl --cpunodebind=0 --localalloc stream -v 1 -M 200M -P 16 >> ~/result_memory/stream/result_${IP} 2>&1
    
    nohup echo ": streamv2 core0-15" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl --cpunodebind=0 --localalloc stream -v 2 -M 200M -P 16 >> ~/result_memory/stream/result_${IP} 2>&1

    nohup echo ": streamv1 core0-31" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl --cpunodebind=0,1 --localalloc stream -v 1 -M 200M -P 32 >> ~/result_memory/stream/result_${IP} 2>&1
    
    nohup echo ": streamv2 core0-31" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl --cpunodebind=0,1 --localalloc stream -v 2 -M 200M -P 32 >> ~/result_memory/stream/result_${IP} 2>&1

    nohup echo ": streamv1 core32-63" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl --cpunodebind=2,3 --localalloc stream -v 1 -M 200M -P 32 >> ~/result_memory/stream/result_${IP} 2>&1
    
    nohup echo ": streamv2 core32-63" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl --cpunodebind=2,3 --localalloc stream -v 2 -M 200M -P 32 >> ~/result_memory/stream/result_${IP} 2>&1

    nohup echo ": streamv1 core0-63" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl --cpunodebind=0,1,2,3 --localalloc stream -v 1 -M 200M -P 64 >> ~/result_memory/stream/result_${IP} 2>&1

    nohup echo ": streamv2 core0-63" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl --cpunodebind=0,1,2,3 --localalloc stream -v 2 -M 200M -P 64 >> ~/result_memory/stream/result_${IP} 2>&1

}

function stream_test_maxcore() {
    cpunum=${MAX_CPUCORES}
    echo ": streamv2cores:${cpunum}"
    if [ ${cpunum} -lt 64 ] ; then
        echo "Currently the maxinum cpucores:${cpunum} is less than 64!!!"
    fi
    nohup echo "V1 Mode......" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl --cpunodebind=0,1,2,3 --localalloc stream -v 1 -M 200M -P ${cpunum} >> ~/result_memory/stream/result_${IP} 2>&1
    sleep 10
    nohup echo "V2 Mode......" >> ~/result_memory/stream/result_${IP} 2>&1
    nohup numactl --cpunodebind=0,1,2,3 --localalloc stream -v 2 -M 200M -P ${cpunum} >> ~/result_memory/stream/result_${IP} 2>&1
    sleep 10
}

#stream_test

for ((i=0;i<2;i++));do
    stream_test_maxcore
done

