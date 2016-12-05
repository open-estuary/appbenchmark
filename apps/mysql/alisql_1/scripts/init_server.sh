#!/bin/bash

#Define global APP_ROOT directory
if [ -z "${APP_ROOT}" ]; then
    # Default value
    APP_ROOT=$(cd `dirname $0` ; cd ../../../../; pwd)
else
    # Re-declare so it can be used in this script
    APP_ROOT=$(echo $APP_ROOT)
fi
export APP_ROOT=${APP_ROOT}

echo "Start AliSql Server ......"

if [ $# -lt 2 ] ; then 
        echo "Usage : ./run_servers.sh {init | start | restart} <inst_num>" 
        exit 0
else 
    default_config="small"
    if [ -z "${3}" ] ; then
        mysql_config=${default_config}
    else
        echo "Config:${3}" 
        mysql_config=${3}
    fi

    echo "Start multi servers ......"
  
    cur_inst=0
    max_inst=${2}
    while [[ ${cur_inst} -lt ${max_inst} ]] 
    do
        ${APP_ROOT}/apps/mysql/alisql_1/scripts/run_single_server.sh ${cur_inst} ${1} ${mysql_config}
        let "cur_inst++"
    done
    
    if [ "${1}" == "init" ] ; then
        cmd_str="Initialize" 
    else 
        cmd_str="Start"
    fi

    echo "${cmd_str} ${1} servers successfully"
fi

