#!/bin/bash

if [ -z "${1}" ] ; then 
    echo "Usage: ./run_test.sh {mariadb_server_IP}"
    exit 0
fi

IP=${1}
CUR_DIR="$(cd `dirname $01`; pwd)"

#############################################################################
# Test Setup
#############################################################################
IRQCPUS="0 7"
MYSQLCPUS="8-63"

service irqbalance stop
#Bind network interrupt to specific cpus
python ../../../toolset/perftools/miscs/set_ethirq_cpu_affinity.py 0 7
echo 1 > /proc/sys/kernel/numa_balancing

#${CUR_DIR}/run_client.sh ${IP} 

#############################################################################
# Execute Test
#############################################################################
PORT=3306
TESTCASES="oltp_simple.lua"
USER="mysql"
PASSWORD="123456"
TEST_RESULT="$(cd ~; pwd)""/apptests/sysbench/sysbench-results"

#Delete old test data if exist
if [ -d "${TEST_RESULT}" ] ; then
    rm -fr ${TEST_RESULT}
fi

TESTCASES="delete.lua insert.lua oltp_complex_ro.lua oltp_complex_rw.lua oltp_simple.lua select.lua update_index.lua update_non_index.lua"
#NUM_THREADS="1 4 8 16 32 64 128"
NUM_THREADS="16"

for TESTCASE in $TESTCASES ; 
do 
    for THREADS in $NUM_THREADS
    do
        ${CUR_DIR}/../../common/scripts/mysql_del_testdb.sh ${IP} ${PORT} ${USER} ${PASSWORD} "/usr/local/mariadb/bin/mysql"
        ${CUR_DIR}/scripts/oltp_test.sh "${IP}" "${USER}" "${PASSWORD}" ${TESTCASE} ${THREADS}  
    done
done

echo "*****************************************************************"
TEST_RESULT="$(cd ~; pwd)""/apptests/sysbench/sysbench-results"
python ${CUR_DIR}/scripts/analysis-sysbench.py  ${TEST_RESULT}

