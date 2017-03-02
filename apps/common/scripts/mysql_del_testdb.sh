#!/bin/bash

HOST="127.0.0.1"
if [ ! -z "${1}" ] ; then
    HOST="${1}"
fi

PORT=3306
if [ ! -z "${2}" ] ; then
    PORT="${2}"
fi

USER="mysql"
if [ ! -z "${3}" ] ; then
    USER="${3}"
fi

PASSWORD="123456"
if [ ! -z "${4}" ] ; then
    PASSWORD="${4}"
fi

MYSQL="mysql"
if [ ! -z "${5}" ] ; then 
    MYSQL="${5}"
fi

${MYSQL} -h "${HOST}" -u ${USER} -P${PORT} -p${PASSWORD} << EOF
drop database if exists sysbench;
drop database if exists sbtest;
create database sysbench;
create database sbtest;
EOF



