#!/bin/bash

##################################################################################################
# Define system variables 
##################################################################################################
YCSB_HOME="/usr/local/ycsb/ycsb-0.12.0"
DATA_HOME=${YCSB_HOME}

IP=${1}
PORT=${2}
THREADS=${3}

TESTDB="ycsb"

CUR_DIR=$(cd `dirname $0`; pwd)
TEST_RESULT=${CUR_DIR}/../testresult/

if [ ! -d ${TEST_RESULT} ] ; then
    mkdir -p ${TEST_RESULT}
fi

source /etc/profile

##################################################################################################
echo "Load data ......"
pushd ${YCSB_HOME} > /dev/null

caselist="a b c d e f"
for casename in ${caselist}
do
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    echo "Start Test Case : ${casename}"
    
    sudo chmod 666 ${DATA_HOME}/workloads/workload${casename}
    #sed -i 's/recordcount=.*/recordcount=200/g' ${DATA_HOME}/workloads/workload${casename}
    #sed -i 's/operationcount=.*/operationcount=2000/g' ${DATA_HOME}/workloads/workload${casename}
 
    sed -i 's/operationcount=.*/operationcount=20000000/g' ${DATA_HOME}/workloads/workload${casename}
    sed -i 's/operationcount=.*/operationcount=20000000/g' ${DATA_HOME}/workloads/workload${casename}

    #Make sure database have been cleared before each test
    ${CUR_DIR}/drop_database.sh
    ${CUR_DIR}/create_database.sh

    ./bin/ycsb load cassandra-cql -s -threads ${THREADS} -p hosts=${1} -p port=${2} -p cassandra.keyspace=${TESTDB} -P ${DATA_HOME}/workloads/workload${casename}
    ./bin/ycsb run cassandra-cql  -s -threads ${THREADS} -p hosts=${1} -p port=${2} -p cassandra.keyspace=${TESTDB}  -P  ${DATA_HOME}/workloads/workload${casename} > ${TEST_RESULT}/test_${casename}_result

done
popd > /dev/null

