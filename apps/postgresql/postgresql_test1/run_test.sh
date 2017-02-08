#!/bin/bash

#Define global APP_ROOT directory

if [ -z "${1}" ] ; then
    echo "Try to connect local host(Note:Use ./run_test.sh <server IP> <pgbench> to connect non-local server"
    IP="127.0.0.1"
else
    IP="${1}"
    echo "Try to connect server-${IP}......"
fi


source /etc/profile

echo "Load system variables ..."
CUR_DIR="$(cd `dirname $0`; pwd)"

if [ ! -f /etc/sysctl.conf ] ; then
    sudo touch /etc/sysctl.conf
    sudo echo "#PostgreSQL Test Configuration" >> /etc/sysctl.conf
    sudo cat ${CUR_DIR}/cofig/sysctl.conf >> /etc/sysctl.conf
else 
    CHECK_STR=$(grep "#PostgreSQL Test Configuration" /etc/sysctl.conf)
    if [ -z "${CHECK_STR}" ] ; then
        sudo cat ${CUR_DIR}/config/sysctl.conf >> /etc/sysctl.conf
    fi
fi 

systcl -p

echo "Initialize database firstly ......"
PGHOME="/usr/local/postgresql"
PGPORT=5432
TESTUSER='postgres'
TESTDB='postgres'

TEST_TYPE="complex"

if [ x"${TEST_TYPE}" == x"basi" ] ; then
    echo "Use pgbench to perform basic test(userid:${TESTUSER}, database:${TESTDB}) ......"

    CLIENT_NUM=128
    TIME_PERIOD=60
    SCALE=1
    TRANSACTION_NUM=1

    echo "Intialize Test Database ..."
    ${PGHOME}/bin/pgbench -h ${IP} -p ${PGPORT} -U ${TESTUSER} -i ${TESTDB} -s ${SCALE}

    echo "Start Benchmark Test ..."
    ${PGHOME}/bin/pgbench -h ${IP} -p ${PGPORT} -U ${TESTUSER} -n -c ${CLIENT_NUM} -s ${SCALE} -t ${TRANSACTION_NUM} -S -T ${TIME_PERIOD} ${TESTDB}

else 
    
    TESTUSER=`whoami`
    echo "Use Pgbench tools to perform complex benchmark tests(userid:${TESTUSER}, database:pgbench,results)..."
    
    PGBENCH_TOOL_DIR="/usr/local/postgresql/pgbench-tools/"
    TESTDIR="${HOME}/apptests/pgbench-tools"
  
    if [ ! -d ${TESTDIR} ] ; then
        mkdir -p ${TESTDIR}
    fi
   
    cp -fr ${PGBENCH_TOOL_DIR}/* ${TESTDIR}
    pushd ${TESTDIR} > /dev/null
    export PATH=${PGHOME}/bin:$PATH

    #sed -i  's/setrandom/set\ random/g' tests/select.sql
    sed -i 's/\\setrandom.*naccounts/\\set\ aid\ random(1,\ \:naccounts)/g' tests/select.sql

    dropdb -h ${IP} -p ${PGPORT} -U ${TESTUSER} results
    dropdb -h ${IP} -p ${PGPORT} -U ${TESTUSER} pgbench
    
    createdb -h ${IP} -p ${PGPORT} -U ${TESTUSER} results
    createdb -h ${IP} -p ${PGPORT} -U ${TESTUSER} pgbench
    
    psql -h ${IP} -p ${PGPORT} -U ${TESTUSER} -f ${PGBENCH_TOOL_DIR}/init/resultdb.sql -d results

    ./newset "Initial Config"
    ./runset 
    
    psql -h ${IP} -p ${PGPORT} -U ${TESTUSER}  -d results -f ${PGBENCH_TOOL_DIR}/reports/report.sql

    #To generate report agains
    #./webreport
    popd > /dev/null
fi
