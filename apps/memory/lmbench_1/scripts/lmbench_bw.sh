#!/bin/bash

echo "******************************************************"
echo "         Lmbench Bandwidth Memory Test                "
echo "******************************************************"

MAX_CPUCORES=$1

perform_bandwidth_test() {
    cmd="$1"
    thread_num=$2
    cpunum="$3"
    
    if [ $thread_num -gt ${MAX_CPUCORES} ] ; then
        return
    fi
        
    for thr_id in ${thread_num}
    do
        for size in 32M  
        do
            for bm in rd frd wr fwr bzero rdwr cp fcp bcopy
            do 
                ${cmd} ${cpunum} bw_mem -P $thr_id -N 5 $size $bm
            done
        done
    done
}

echo "*******************core0***********************"
perform_bandwidth_test "taskset -c" 1 0 

echo "*******************cluster0********************"
#perform_bandwidth_test "taskset -c" 4 "0-3"

echo "*******************Die0************************"
#perform_bandwidth_test "taskset -c" 16 "0-15"

echo "*******************1P*************************"
#perform_bandwidth_test "taskset -c" 32 "0-31"

echo "*******************core0 Ta->Tb****************"
#perform_bandwidth_test "numactl -m 1 -C" 1 0 

echo "*******************cluster0 Ta->Tb*************"
#perform_bandwidth_test "numactl -m 1 -C" 4 "0-3" 

echo "*******************Die0 Ta->Tb*****************"
#perform_bandwidth_test "numactl -m 1 -C" 16 "0-15"

