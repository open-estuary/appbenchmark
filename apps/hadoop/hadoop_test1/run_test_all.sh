#!/bin/bash

trap "" SIGTTOU

source /etc/profile
#sysctl -p
echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo 0 > /proc/sys/kernel/numa_balancing
echo 0 > /proc/sys/kernel/sched_autogroup_enabled
echo 0 > /proc/sys/vm/swappiness
echo 3000 > /proc/sys/vm/dirty_expire_centisecs
echo 80 > /proc/sys/vm/dirty_ratio
echo 10240 > /proc/sys/net/core/somaxconn

#Stop irqbalance firstly
service irqbalance stop

delete_tmp_dirs() {
    retry_num=0
    while [[ ${retry_num} -lt 2 ]]
    do
        hdfs dfs -rm -r /HiBench
        let "retry_num++"
        sleep 15
    done
}

max_iter_num=3
iter_index=0
while [[ ${iter_index} -lt ${max_iter_num} ]]
do
    caseslist=( "kmeans" "dfsioe" "pagerank" "terasort" "sort" "scan" "wordcount")
    numa_balancing=("0"  "0"      "1"         "0"        "0"    "0"    "1")

    #caseslist=("kmeans" "scan" "sort" "terasort" "wordcount" "dfsioe") 
    #caseslist=("kmeans" "dfsioe" "pagerank")
    
    case_index=0
    for casename in ${caseslist[@]} 
    do
        echo ${numa_balancing[${case_index}]} > /proc/sys/kernel/numa_balancing
        echo "${casename} has numa_balancing :$(cat /proc/sys/kernel/numa_balancing)"
        #delete_tmp_dirs
        ./run_client.sh ${casename} large    
        
        delete_tmp_dirs
        ./run_client.sh ${casename} huge 
        let "case_index++"
    done

    let "iter_index++"
done

