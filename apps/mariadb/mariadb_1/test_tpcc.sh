#!/bin/bash

if [ -z "${1}" ] ; then 
    echo "Usage: ./run_test.sh {mariadb_server_IP}"
    exit 0
fi

IP=${1}

CUR_DIR="$(cd `dirname $01`; pwd)"

${CUR_DIR}/../../mysql/common/scripts/percona_tpcc_test.sh /usr/local/mariadb/bin/mysql ${IP}

