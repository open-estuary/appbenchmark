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

echo "**********************************************"
echo "        Lmbench BandWidth Memory Test         "
echo "**********************************************"

perform_bandwidth_test() {
    cmd="$1"
    thread_num=$2
    cpunum="$3"
    
    if [ $thread_num -gt ${CPUCORES} ] ; then
        return
    fi

    nohup echo "*************************" ${cmd} ${thread_num} "***************************" >> ~/result_memory/bw/result_${IP} 2>&1

    for thr_id in ${thread_num}
    do
        for size in 32M  
        do
            for bm in rd frd wr fwr bzero rdwr cp fcp bcopy
            do 
              nohup ${cmd} ${cpunum} bw_mem -P $thr_id -N 5 $size $bm >> ~/result_memory/bw/result_${IP} 2>&1
            done
        done
    done

}

echo "*******************core0***********************"
perform_bandwidth_test "taskset -c" 1 0

echo "*******************cluster0********************"
perform_bandwidth_test "taskset -c" 4 "0-3"

echo "*******************Die0************************"
perform_bandwidth_test "taskset -c" 16 "0-15"

echo "*******************1P*************************"
perform_bandwidth_test "taskset -c" 32 "0-31"

echo "*******************core0 Ta->Tb****************"
perform_bandwidth_test "numactl -m 1 -C" 1 0 

echo "*******************cluster0 Ta->Tb*************"
perform_bandwidth_test "numactl -m 1 -C" 4 "0-3" 

echo "*******************Die0 Ta->Tb*****************"
perform_bandwidth_test "numactl -m 1 -C" 16 "0-15"

