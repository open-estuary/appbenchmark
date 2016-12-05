#!/bin/bash

if [ $# -lt 4 ]; then
    echo "Usage: client_start.sh <ip_address> <username> <userpassword> <action> [port/inst_num]"
    echo "By default, it will connect 3306 port"
    exit 0
fi

BASE_DIR=$(cd ~; pwd)
PORT_ID=3306
if [ $# -ge 5 ] ; then
    PORT_ID=${5}
fi

######################################################################################
# Notes:
#  To start client tests
#  Usage: client_start.sh <mysql_server_ip> <username> <passwd> {inital}
#####################################################################################

default_table_count=8
default_table_size=100000

if [ "$4" == "init" ] ; then
    #Step 1: Prepare data
    sysbench --test=${BASE_DIR}/apptests/sysbench/tests/db/parallel_prepare.lua \
        --oltp-test-mode=complex  \
        --mysql-host=$1 --mysql-db=sysbench \
        --mysql-password=$3 \
        --max-time=7200 --max-requests=0 --mysql-user=$2 \
        --mysql-table-engine=innodb --oltp-table-size=${default_table_size} \
        --oltp-tables-count=${default_table_count} --rand-type=special --rand-spec-pct=100 \
        --num-threads=10 \
        --mysql-port=${PORT_ID} prepare
    
    exit 1

    #Step 2: Initialize tables
    sysbench --test=${BASE_DIR}/apptests/sysbench/tests/db/parallel_prepare.lua \
        --oltp-test-mode=complex  \
        --mysql-host=$1 --mysql-db=sysbench \
        --mysql-password=$3 \
        --max-time=7200 --max-requests=0 --mysql-user=$2 \
        --mysql-table-engine=innodb --oltp-table-size=${default_table_size} \
        --oltp-tables-count=${default_table_count} --rand-type=special --rand-spec-pct=100 \
        --num-threads=10 \
        --mysql-port=${PORT_ID} run
elif [ "$4" == "test" ] ; then
    inst_num=${5}
    thread_per_inst=1
    if [ ! -z "${6} " ] ; then
        thread_per_inst=${6}
    fi

    ${APP_ROOT}/apps/mysql/alisql_1/scripts/sysbench_lots_instances.sh ${1} ${inst_num} ${thread_per_inst}
else 
    echo "argument should be {init | loaddata | test} "
fi

echo "**********************************************************************************"
echo "start tcprstat completed"

