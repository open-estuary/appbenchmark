#!/bin/bash

BASE_DIR=$(cd ~; pwd)

INST_NUM=${2}
start_port=3306
inst_index=0
mysql_ports=""

while [[ ${inst_index} -lt ${INST_NUM} ]]
do
    if [ -z "${mysql_ports}" ] ; then
        mysql_ports=${start_port}
    else 
        mysql_ports=${mysql_ports},${start_port}
    fi
    let "start_port++"
    let "inst_index++"
done

threads_num=200

if [ ! -z "${3}" ] ; then
    let "threads_num=200*${3}"
fi

#echo "mysql:${mysql_ports}"

sysbench  ${BASE_DIR}/apptests/sysbench/tests/db/oltp.lua \
                 --oltp-table-size=100000 \
		 --oltp-tables-count=8 \
		 --mysql-db=sysbench \
		 --report-interval=1 \
		 --max-requests=0 \
		 --max-time=7200 \
		 --mysql-host=${1} \
                 --mysql-port=${mysql_ports} \
                 --mysql-password='123456' \
		 --mysql-user=root \
		 --mysql-table-engine=innodb \
                 --num-threads=${threads_num} run 

