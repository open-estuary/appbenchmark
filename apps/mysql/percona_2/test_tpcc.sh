#!/bin/bash

if [ -z "${1}" ] ; then 
    echo "Usage: ./run_test.sh {percona_server_IP}"
    exit 0
fi

IP=${1}

CUR_DIR="$(cd `dirname $01`; pwd)"

${CUR_DIR}/../../mysql/common/scripts/percona_tpcc_test.sh /u01/my3306/bin/mysql ${IP}

