#!/bin/bash

##################################################################################################
# Define system variables 
##################################################################################################
YCSB_HOME="/usr/local/ycsb/ycsb-0.12.0"
MONGODATA_HOME="/usr/local/ycsb/ycsb_data/ycsb-mongodb"
MONGOBIN="/usr/local/mongodb/bin"

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
    
    sudo chmod 666 ${MONGODATA_HOME}/workloads/workload${casename}
    #sed -i 's/recordcount=.*/recordcount=200/g' ${MONGODATA_HOME}/workloads/workload${casename}
    #sed -i 's/operationcount=.*/operationcount=2000/g' ${MONGODATA_HOME}/workloads/workload${casename}
 
    sed -i 's/operationcount=.*/operationcount=20000000/g' ${MONGODATA_HOME}/workloads/workload${casename}
    sed -i 's/operationcount=.*/operationcount=20000000/g' ${MONGODATA_HOME}/workloads/workload${casename}
    #Make sure database have been cleared before each test
    ${MONGOBIN}/mongo ${IP}:${PORT}/${TESTDB} ${CUR_DIR}/delete_ycsb.js

    ./bin/ycsb load mongodb -s -threads ${THREADS} -p mongodb.url=mongodb://${IP}:${PORT}/${TESTDB}  -P ${MONGODATA_HOME}/workloads/workload${casename}
    ./bin/ycsb run mongodb -s -threads ${THREADS} -p mongodb.url=mongodb://${IP}:${PORT}/${TESTDB}  -P  ${MONGODATA_HOME}/workloads/workload${casename} > ${TEST_RESULT}/test_${casename}_result
done

popd > /dev/null

