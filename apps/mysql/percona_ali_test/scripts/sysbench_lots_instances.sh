#!/bin/bash

BASE_DIR=$(cd ~; pwd)

INST_NUM=${1}
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

#echo "mysql:${mysql_ports}"

sysbench  --test=${BASE_DIR}/apptests/sysbench/tests/db/oltp.lua \
                 --oltp-table-size=100000 \
		 --oltp-tables-count=8 \
		 --mysql-db=sysbench \
		 --report-interval=1 \
		 --max-requests=0 \
		 --max-time=7200 \
		 --mysql-host=127.0.0.1 \
                 --mysql-port=${mysql_ports} \
                 --mysql-password='123456' \
		 --mysql-user=root \
		 --mysql-table-engine=innodb \
                 --num-threads=1 run 

