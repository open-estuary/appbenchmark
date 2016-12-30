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
echo 5000000 > /proc/sys/kernel/sched_migration_cost_ns

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

#Enable numa_balancing
max_iter_num=3
iter_index=0
while [[ ${iter_index} -lt ${max_iter_num} ]]
do
    #caseslist=( "pagerank" "wordcount" "dfsioe"  "terasort" "sort"  "scan" "kmeans")
    #numa_balancing=( "1"      "1"         "0"         "0"   "0"     "0"    "0")

    caseslist=("kmeans" "wordcount" "pagerank") 
    #caseslist=("kmeans")
 
    case_index=0
    for casename in ${caseslist[@]} 
    do
        echo 1 > /proc/sys/kernel/numa_balancing
        echo 60000 > /proc/sys/kernel/numa_balancing_scan_period_max_ms
        echo "${casename} has numa_balancing :$(cat /proc/sys/kernel/numa_balancing)"

        delete_tmp_dirs
        ./scripts/start_client.sh ${casename} large    
        
        delete_tmp_dirs
        ./scripts/start_client.sh ${casename} huge 
        let "case_index++"
    done

    let "iter_index++"
done

#Disable numa_balancing
max_iter_num=3
iter_index=0
while [[ ${iter_index} -lt ${max_iter_num} ]]
do
    caseslist=("dfsioe"  "terasort" "sort"  "scan")

    #caseslist=("kmeans" "scan" "sort" "terasort" "wordcount" "dfsioe") 
 
    case_index=0
    for casename in ${caseslist[@]} 
    do
        echo 0 > /proc/sys/kernel/numa_balancing
        echo 1000 > /proc/sys/kernel/numa_balancing_scan_period_max_ms
        echo "${casename} has numa_balancing :$(cat /proc/sys/kernel/numa_balancing)"

        delete_tmp_dirs
        ./scripts/start_client.sh ${casename} large    
        
        delete_tmp_dirs
        ./scripts/start_client.sh ${casename} huge 
        let "case_index++"
    done

    let "iter_index++"
done

#Restore to default values
echo 60000 > /proc/sys/kernel/numa_balancing_scan_period_max_ms
echo 1 > /proc/sys/kernel/numa_balancing
echo 1 > /proc/sys/kernel/sched_autogroup_enabled
echo 0 > /proc/sys/vm/swappiness
echo 500000 > /proc/sys/kernel/sched_migration_cost_ns
