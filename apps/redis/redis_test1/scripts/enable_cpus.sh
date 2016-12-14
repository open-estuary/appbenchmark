#!/bin/bash

start_cpu_num=${1}
end_cpu_num=${2}
enable=${3}

while [[ ${start_cpu_num} -lt ${end_cpu_num} ]] 
do
    if [[ ${enable} -eq 1 ]]; then
        echo "cpu${start_cpu_num} is online"
    else 
        echo "cpu${start_cpu_num} is offline"
    fi

    echo ${enable} > /sys/devices/system/cpu/cpu${start_cpu_num}/online
    let "start_cpu_num++"
done
