#!/bin/bash

MYSQL="mysql"
if [ ! -z "${1}" ] ; then
    MYSQL=$1
fi

HOST="127.0.0.1"
if [ ! -z "${2}" ] ; then
    HOST="${2}"
fi

USER="mysql"
if [ ! -z "${3}" ] ; then
    USER="${3}"
fi

PASSWORD="123456"
if [ ! -z "${4}" ] ; then
    PASSWORD="${4}"
fi

INSTALL_DIR="/usr/local/percona-tpcc-mysql/tpcc-mysql"
MYSQL_CMD="${MYSQL} -h${HOST} -p${PASSWORD} -P3306 -u${USER}"
DATABASE="tpcc1000"
CONNECTION=1
WAREHOUSE=1
WARMUPTIME=10
RUNTIME=10800

pushd ${INSTALL_DIR}

${MYSQL_CMD}  << EOF
drop database if exists ${DATABASE};
create database if not exists ${DATABASE}; 
EOF

${MYSQL_CMD} ${DATABASE} < create_table.sql
${MYSQL_CMD} ${DATABASE} < add_fkey_idx.sql

./tpcc_load -h${HOST} -d ${DATABASE} -u${USER} -p${PASSWORD} -w ${WAREHOUSE}
./tpcc_start -h${HOST} -d ${DATABASE} -u${USER} -p${PASSWORD} -w ${WAREHOUSE} -c${CONNECTION} -r${WARMUPTIME} -l${RUNTIME} -i1

