#!/bin/bash

trap "" SIGTTOU

source /etc/profile

delete_tmp_dirs() {
    retry_num=0
    while [[ ${retry_num} -lt 6 ]]
    do
        hdfs dfs -rm -r /HiBench
        let "retry_num++"
        sleep 60
    done
}


caseslist=("wordcount" "terasort" "kmeans" "pagerank" "scan" "sort" "dfsioe")
for casename in ${caseslist[@]} 
do 
    echo "Begin to test ${casename}"
    ./run_client.sh ${casename} large    
    ./run_client.sh ${casename} huge
   
    delete_tmp_dirs
done

