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

if [ "$4" == "init" ] ; then
    #default_table_count=100
 
    default_table_count=8

    #Step 1: Prepare data
    sysbench --test=${BASE_DIR}/apptests/sysbench/tests/db/parallel_prepare.lua \
        --oltp-test-mode=complex  \
        --mysql-host=$1 --mysql-db=sysbench \
        --mysql-password=$3 \
        --max-time=7200 --max-requests=0 --mysql-user=$2 \
        --mysql-table-engine=innodb --oltp-table-size=1000000 \
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
        --mysql-table-engine=innodb --oltp-table-size=1000000 \
        --oltp-tables-count=${default_table_count} --rand-type=special --rand-spec-pct=100 \
        --num-threads=10 \
        --mysql-port=${PORT_ID} run
elif [ "$4" == "loaddata" ] ; then
#Step 3: Run test case
${APP_ROOT}/apps/mysql/percona_ali_test/scripts/readall.sh $1 $2 $3  47 3306
elif [ "$4" == "test" ] ; then
#${APP_ROOT}/apps/mysql/percona_ali_test/scripts/sysbench.sh $1 $2 $3 on 1 90 $1 sysbench 100 1000000 select6 20000
#${APP_ROOT}/apps/mysql/percona_ali_test/scripts/sysbench.sh $1 $2 $3 on 2 180 $1 sysbench 100 1000000 select6 20000
#${APP_ROOT}/apps/mysql/percona_ali_test/scripts/sysbench.sh $1 $2 $3 on 3 270 $1 sysbench 100 1000000 select6 20000
#${APP_ROOT}/apps/mysql/percona_ali_test/scripts/sysbench.sh $1 $2 $3 on 4 360  $1 sysbench 100 1000000 select6 20000
#${APP_ROOT}/apps/mysql/percona_ali_test/scripts/sysbench.sh $1 $2 $3 on 50 450 $1 sysbench 100 1000000 select6 100000
#${APP_ROOT}/apps/mysql/percona_ali_test/scripts/sysbench.sh $1 $2 $3 on 500 450 $1 sysbench 100 1000000 select6 100000

inst_num=${5}
${APP_ROOT}/apps/mysql/percona_ali_test/scripts/sysbench_lots_instances.sh ${inst_num}

else 
    echo "argument should be {init | loaddata | test} "
fi

echo "**********************************************************************************"
echo "start tcprstat completed"

