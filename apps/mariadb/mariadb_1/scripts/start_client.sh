#!/bin/bash

if [ $# -lt 4 ]; then
    echo "Usage: client_start.sh <ip_address> <username> <userpassword> <action> [port]"
    echo "By default, it will connect 3306 port"
    exit 0
fi

BASE_DIR=$(cd ~; pwd)

PORT_ID=3306
TESTCASE="select6"

if [ $# -ge 5 ] ; then
    PORT_ID=${5}
fi

if [ ! -z "${6}" ] ; then
    TESTCASE="%{6}"
fi

######################################################################################
# Notes:
#  To start client tests
#  Usage: client_start.sh <mysql_server_ip> <username> <passwd> {inital}
#####################################################################################

default_table_count=100
default_table_size=1000000
if [ "$4" == "init" ] ; then

    #Step 1: Prepare data
    sysbench ${BASE_DIR}/apptests/sysbench/tests/db/parallel_prepare.lua \
    --db-driver=mysql \
        --oltp-test-mode=complex  \
        --mysql-host=$1 --mysql-db=sysbench \
        --mysql-password=$3 \
        --time=7200 --max-requests=0 --mysql-user=$2 \
        --mysql-table-engine=innodb --oltp-table-size=${default_table_size} \
        --oltp-tables-count=${default_table_count} --rand-type=special --rand-spec-pct=100 \
        --threads=10 \
        --mysql-port=${PORT_ID} prepare

    #Step 2: Initialize tables
    sysbench ${BASE_DIR}/apptests/sysbench/tests/db/parallel_prepare.lua \
    --db-driver=mysql \
        --oltp-test-mode=complex  \
        --mysql-host=$1 --mysql-db=sysbench \
        --mysql-password=$3 \
        --time=7200 --max-requests=0 --mysql-user=$2 \
        --mysql-table-engine=innodb --oltp-table-size=${default_table_size} \
        --oltp-tables-count=${default_table_count} --rand-type=special --rand-spec-pct=100 \
        --threads=10 \
        --mysql-port=${PORT_ID} run
elif [ "$4" == "loaddata" ] ; then
    #Step 3: Warm up
    ${APP_ROOT}/apps/mariadb/mariadb_1/scripts/readall.sh $1 $2 $3  ${default_table_count}

elif [ "$4" == "test" ] ; then
    #Step 4: Run real testcase
    active_num=50
    unactive_num=450
    multi_index=1
    if [ ! -z "${5}" ] ; then
        multi_index=$5
    fi
    let "active_num=active_num*multi_index"
    let "unactive_num=unactive_num*multi_index"

    echo "start active connections:${active_num}, unactive connections:${unactive_num}"
    ${APP_ROOT}/apps/mariadb/mariadb_1/scripts/sysbench.sh $1 $2 $3 on ${active_num} ${unactive_num} $1 sysbench 100 1000000 ${TESTCASE} 100000

else 
    echo "argument should be {init | loaddata | test} "
fi

echo "**********************************************************************************"
echo "start tcprstat completed"

