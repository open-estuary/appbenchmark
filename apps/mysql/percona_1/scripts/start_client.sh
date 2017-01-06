#!/bin/bash

if [ $# -lt 4 ]; then
    echo "Usage: client_start.sh <ip_address> <username> <userpassword> <action> [port]"
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

default_table_count=100
default_table_size=1000000
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
elif [ "$4" == "loaddata" ] ; then
#Step 3: Run test case

#${APP_ROOT}/apps/mysql/percona_1/scripts/readall.sh $1 $2 $3  47 3306
${APP_ROOT}/apps/mysql/percona_1/scripts/readall.sh $1 $2 $3  ${default_table_count}

elif [ "$4" == "test" ] ; then
#${APP_ROOT}/apps/mysql/percona_1/scripts/sysbench.sh $1 $2 $3 on 1 90 $1 sysbench 100 1000000 select6 20000
#${APP_ROOT}/apps/mysql/percona_1/scripts/sysbench.sh $1 $2 $3 on 2 180 $1 sysbench 100 1000000 select6 20000
#${APP_ROOT}/apps/mysql/percona_1/scripts/sysbench.sh $1 $2 $3 on 3 270 $1 sysbench 100 1000000 select6 20000
#${APP_ROOT}/apps/mysql/percona_1/scripts/sysbench.sh $1 $2 $3 on 4 360  $1 sysbench 100 1000000 select6 20000

active_num=50
unactive_num=450
multi_index=1
if [ ! -z "${5}" ] ; then
    multi_index=$5
fi
let "active_num=active_num*multi_index"
let "unactive_num=unactive_num*multi_index"

echo "start active connections:${active_num}, unactive connections:${unactive_num}"
${APP_ROOT}/apps/mysql/percona_1/scripts/sysbench.sh $1 $2 $3 on ${active_num} ${unactive_num} $1 sysbench 100 1000000 select6 100000
#${APP_ROOT}/apps/mysql/percona_1/scripts/sysbench.sh $1 $2 $3 on 100  900 $1 sysbench 100 1000000 select6 100000
#sleep 60

#${APP_ROOT}/apps/mysql/percona_1/scripts/sysbench.sh $1 $2 $3 on 150  1350 $1 sysbench 100 1000000 select6 100000
#sleep 60

#${APP_ROOT}/apps/mysql/percona_1/scripts/sysbench.sh $1 $2 $3 on 200 1800 $1 sysbench 100 1000000 select6 100000
#sleep 60
#${APP_ROOT}/apps/mysql/percona_1/scripts/sysbench.sh $1 $2 $3 on 250 2250  $1 sysbench 100 1000000 select6 100000
#sleep 60

#${APP_ROOT}/apps/mysql/percona_1/scripts/sysbench.sh $1 $2 $3 on 300 2700 $1 sysbench 100 1000000 select6 100000
#sleep 60
#${APP_ROOT}/apps/mysql/percona_1/scripts/sysbench.sh $1 $2 $3 on 350 3150 $1 sysbench 100 1000000 select6 100000

else 
    echo "argument should be {init | loaddata | test} "
fi

echo "**********************************************************************************"
echo "start tcprstat completed"

