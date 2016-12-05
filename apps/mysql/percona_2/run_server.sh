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
    ${APP_ROOT}/apps/mysql/percona_ali_2/scripts/run_single_server.sh
else 
    if [ $# -lt 2 ] ; then 
        echo "Usage : ./run_servers.sh {init | start} <inst_num>" 
        exit 0
    fi

    echo "Start multi servers ......"
    tmp_mysql_init_file="/tmp/mysql_init_cmd_"${RANDOM}
    rm ${tmp_mysql_init_file}
    touch ${tmp_mysql_init_file}
    echo > ${tmp_mysql_init_file}
  
    cur_inst=0
    max_inst=${2}
    while [[ ${cur_inst} -lt ${max_inst} ]] 
    do
        #echo "${APP_ROOT}/apps/mysql/percona_ali_2/scripts/run_single_server.sh ${cur_inst} $@" >> ${tmp_mysql_init_file}
        ${APP_ROOT}/apps/mysql/percona_ali_2/scripts/run_single_server.sh ${cur_inst} ${1}
        let "cur_inst++"
    done

    #${APP_ROOT}/toolset/util/parallel_cmds.py ${tmp_mysql_init_file}
    rm ${tmp_mysql_init_file}
    
    if [ "${2}" == "init" ] ; then
        cmd_str="Initialize" 
    else 
        cmd_str="Start"
    fi

    echo "${cmd_str} ${1} servers successfully"
fi

