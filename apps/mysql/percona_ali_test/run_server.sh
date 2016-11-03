#!/bin/bash

#Define global APP_ROOT directory
if [ -z "${APP_ROOT}" ]; then
    # Default value
    APP_ROOT=$(cd `dirname $0` ; cd ../../../; pwd)
else
    # Re-declare so it can be used in this script
    APP_ROOT=$(echo $APP_ROOT)
fi
export APP_ROOT=${APP_ROOT}

echo "Start Percona Server ......"

if [ $# -lt 1 ] ; then 
    echo "Start single server ......"
    ${APP_ROOT}/apps/mysql/percona_ali_test/scripts/run_single_server.sh
else 
    echo "Start multi servers ......"
    tmp_mysql_init_file="/tmp/mysql_init_cmd_"${RANDOM}
    rm ${tmp_mysql_init_file}
    touch ${tmp_mysql_init_file}
    echo > ${tmp_mysql_init_file}
  
    cur_inst=0
    max_inst=${1}
    while [[ ${cur_inst} -lt ${max_inst} ]] 
    do
        echo "${APP_ROOT}/apps/mysql/percona_ali_test/scripts/run_single_server.sh ${cur_inst}" >> ${tmp_mysql_init_file}
        let "cur_inst++"
    done

    ${APP_ROOT}/toolset/util/parallel_cmds.py ${tmp_mysql_init_file}
    rm ${tmp_mysql_init_file}
    
    echo "Start ${1} servers successfully"
fi

