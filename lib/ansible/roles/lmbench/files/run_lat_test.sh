#!/bin/bash

if [ -z "$(which stream 2>/dev/null)" ] ; then
    echo "Please install stream firstly !"
    exit 0
fi

if [ -z "$(echo $PATH | grep '/usr/local/bin')" ] ; then
    export PATH=$PATH:/usr/local/bin
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

MAX_CPUCORES=$CPUCORES

#nohup echo "****************** 32 cores t ***********************" >> ~/result_memory/lat/result_${IP} 2>&1
#nohup numactl --physcpubind=0-31 --localalloc lat_mem_rd -P 1 -t 32M 128 >> ~/result_memory/lat/result_${IP} 2>&1

for ctype in  "t" #"s"
do
    for cpunum in 1 4 16 32 
    do
        nohup echo "****************** ${cpunum} cores ${ctype} ***********************" >> ~/result_memory/lat/result_${IP} 2>&1
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
           nohup numactl --localalloc --physcpubind=0 lat_mem_rd -P 1 ${flags} 256M 128 >> ~/result_memory/lat/result_${IP} 2>&1
        else
           nohup numactl --localalloc --physcpubind=0-${cpuindex} lat_mem_rd -P ${cpunum} ${flags} 256M 128  >> ~/result_memory/lat/result_${IP} 2>&1
        fi
    done
done

for ctype in  "t" #"s"
do
    for cpunum in 1 4 16 
    do
        nohup echo "******************1P TA->TC ${cpunum} cores ${ctype} ***********************" >> ~/result_memory/lat/result_${IP} 2>&1
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
           nohup numactl --membind=1 --physcpubind=0 lat_mem_rd -P 1 ${flags} 256M 128  >> ~/result_memory/lat/result_${IP} 2>&1
        else
           nohup numactl --membind=1 --physcpubind=0-${cpuindex} lat_mem_rd -P ${cpunum} ${flags} 256M 128  >> ~/result_memory/lat/result_${IP} 2>&1
        fi
    done
done 

exit 0
