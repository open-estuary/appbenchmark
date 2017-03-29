#!/bin/bash

#Define global APP_ROOT directory

if [ -z "${1}" ] ; then
    echo "Try to connect local host(Note:Use ./run_test.sh <server IP> <testcase>"
    IP="127.0.0.1"
else
    IP="${1}"
    echo "Try to connect server-${IP}......"
fi


source /etc/profile

CASSANDRA_HOME="/usr/local/cassandra"
NUMBER_OPS=2000000
THREADS=50
CUR_DIR=$(cd `dirname $0`; pwd)

TESTCASE="${2}"

if [ -z ${TESTCASE} ] || [ "${TESTCASE}" == "write" ] ; then
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    echo "Perform write stress test..."
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    ${CASSANDRA_HOME}/tools/bin/cassandra-stress write n=${NUMBER_OPS} -node ${IP} -rate threads=${THREADS}
fi

if [ -z ${TESTCASE} ] || [ "${TESTCASE}" == "read" ] ; then
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    echo "Perform read stress test..."
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    ${CASSANDRA_HOME}/tools/bin/cassandra-stress read n=${NUMBER_OPS} -node ${IP} -rate threads=${THREADS}
fi

if [ -z ${TESTCASE} ] || [ "${TESTCASE}" == "mixed" ] ; then
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    echo "Perform mixed stress test..."
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    ${CASSANDRA_HOME}/tools/bin/cassandra-stress mixed n=${NUMBER_OPS} -node ${IP} -rate threads=${THREADS}
fi

if [ -z ${TESTCASE} ] || [ "${TESTCASE}" == "user" ] ; then
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    echo "Perform User-defined profile stress test..."
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    ${CASSANDRA_HOME}/tools/bin/cassandra-stress user profile=${CUR_DIR}/scripts/cqlstress.yaml n=${NUMBER_OPS} ops\(insert=3,read1=1\) -node ${IP}
fi

