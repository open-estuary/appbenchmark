#!/bin/bash

MAX_CPUCORES=$1
function stream_test() {
    echo ": streamv1 core0"
    numactl -C 0 --localalloc stream -v 1 -M 200M -P 1

    echo ": streamv2 core0"
    numactl -C 0 --localalloc stream -v 2 -M 200M -P 1

    echo ": streamv1 core0-3"
    numactl -C 0-3 --localalloc stream -v 1 -M 200M -P 4
    
    echo ": streamv2 core0-3"
    numactl -C 0-3 --localalloc stream -v 2 -M 200M -P 4

    echo ": streamv1 core0-15"
    numactl --cpunodebind=0 --localalloc stream -v 1 -M 200M -P 16
    
    echo ": streamv2 core0-15"
    numactl --cpunodebind=0 --localalloc stream -v 2 -M 200M -P 16

    echo ": streamv1 core0-31"
    numactl --cpunodebind=0,1 --localalloc stream -v 1 -M 200M -P 32
    
    echo ": streamv2 core0-31"
    numactl --cpunodebind=0,1 --localalloc stream -v 2 -M 200M -P 32

    echo ": streamv1 core32-63"
    numactl --cpunodebind=2,3 --localalloc stream -v 1 -M 200M -P 32
    
    echo ": streamv2 core32-63"
    numactl --cpunodebind=2,3 --localalloc stream -v 2 -M 200M -P 32

    echo ": streamv1 core0-63"
    numactl --cpunodebind=0,1,2,3 --localalloc stream -v 1 -M 200M -P 64

    echo ": streamv2 core0-63"
    numactl --cpunodebind=0,1,2,3 --localalloc stream -v 2 -M 200M -P 64
}

function stream_test_maxcore() {
    cpunum=${MAX_CPUCORES}
    echo ": streamv2cores:${cpunum}"
    if [ ${cpunum} -lt 64 ] ; then
        echo "Currently the maxinum cpucores:${cpunum} is less than 64!!!"
    fi
    echo "V1 Mode......"
    numactl --cpunodebind=0,1,2,3 --localalloc stream -v 1 -M 200M -P ${cpunum}
    echo "V2 Mode......"
    numactl --cpunodebind=0,1,2,3 --localalloc stream -v 2 -M 200M -P ${cpunum}
}

stream_test

for ((i=0;i<10;i++));do
    stream_test_maxcore
done



