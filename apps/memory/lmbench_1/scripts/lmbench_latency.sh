#!/bin/bash

MAX_CPUCORES=$1

for ctype in "t" "s"
do
    for cpunum in 1 4 16 32 
    do
        echo "****************** ${cpunum} cores ${ctype} ***********************"
        if (( ${cpunum} > ${MAX_CPUCORES} )) ; then
            continue
        fi

        flags=""
        if [ "${ctype}" == "t" ] ; then
            flags="-t"
        fi 
        cpuindex=${cpunum}
        let "cpuindex=cpuindex-1"
        if [ ${cpunum} -eq 1 ] ; then
           numactl --membind=0 --physcpubind=0 lat_mem_rd -P 1 ${flags} 32M 128
        else
           numactl --membind=0 --physcpubind=0-${cpuindex} lat_mem_rd -P ${cpunum} ${flags} 32M 128
        fi
    done
done

for ctype in "t" "s"
do
    for cpunum in 1 4 16 32 
    do
        echo "******************1P TA->TC ${cpunum} cores ${ctype} ***********************"
        if [ ${cpunum} -gt ${MAX_CPUCORES} ] ; then
            continue
        fi

        flags=""
        if [ "${ctype}" == "t" ] ; then
            flags="-t"
        fi 
        cpuindex=${cpunum}
        let "cpuindex=cpuindex-1"
        if [ ${cpunum} -eq 1 ] ; then
           numactl --membind=1 --physcpubind=0 lat_mem_rd -P 1 ${flags} 32M 128
        else
           numactl --membind=1 --physcpubind=0-${cpuindex} lat_mem_rd -P ${cpunum} ${flags} 32M 128
        fi
    done
done
exit 0

echo '*********************1core t*************************'
numactl --membind=0 --physcpubind=0 lat_mem_rd -P 1 -t 32M 128
echo '*********************4core t*************************'
numactl --membind=0 --physcpubind=0-3 lat_mem_rd -P 4 -t 32M 128
echo '*********************16core t*************************'
numactl --membind=0 --physcpubind=0-15 lat_mem_rd -P 16 -t 32M 128
echo '*********************1P t*************************'
numactl --membind=0 --physcpubind=0-31 lat_mem_rd -P 32 -t 32M 128


echo '*********************1core s*************************'
numactl --membind=0 --physcpubind=0 lat_mem_rd -P 1 32M 128
echo '*********************4core s*************************'
numactl --membind=0 --physcpubind=0-3 lat_mem_rd -P 4 32M 128
echo '*********************16core s*************************'
numactl --membind=0 --physcpubind=0-15 lat_mem_rd -P 16 32M 128
echo '*********************1P s*************************'
numactl --membind=0 --physcpubind=0-31 lat_mem_rd -P 32 32M 128


echo '*********************1P TA->TC 1core t******************'
numactl --membind=1 --physcpubind=0 lat_mem_rd -P 1 -t 32M 128
echo '*********************1P TA->TC 4core t*******************'
numactl --membind=1 --physcpubind=0-3 lat_mem_rd -P 4 -t 32M 128
echo '*********************1P TA->TC 16core t*******************'
numactl --membind=1 --physcpubind=0-15 lat_mem_rd -P 16 -t 32M 128

echo '*********************1P TA->TC 1core s******************'
numactl --membind=1 --physcpubind=0 lat_mem_rd -P 1 32M 128
echo '*********************1P TA->TC 4core s*******************'
numactl --membind=1 --physcpubind=0-3 lat_mem_rd -P 4 32M 128
echo '*********************1P TA->TC 16core s*******************'
numactl --membind=1 --physcpubind=0-15 lat_mem_rd -P 16 32M 128

