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

echo "Start to setup client"
CUR_DIR="$(cd `dirname $0`; pwd)"
${CUR_DIR}/setup.sh client

#ip="192.168.1.187"

ip="127.0.0.1"
userid="root"
password="123456"

if [ $# -lt 1 ] ; then 
    echo "Usage: ./run_test.sh {init | loaddata | test} [inst_num]"
    exit 0
fi


if [ $# -ge 2 ] ; then
   echo "Try to test multi mysql instances[$2]......"

   inst_num=${2}
   if [ "x${1}" == "xtest" ] ; then
      ${APP_ROOT}/apps/mysql/percona_2/scripts/start_client.sh ${ip} \
               ${userid} ${password} ${1} ${inst_num}
   else    

       port_num=3306
       cur_inst=0
       while [[ ${cur_inst} -lt ${inst_num} ]]
       do
           ${APP_ROOT}/apps/mysql/percona_2/scripts/start_client.sh ${ip} \
               ${userid} ${password} ${1} ${port_num}
           let "port_num++"
           let "cur_inst++"
       done
   fi
else 
    #Include common setup utility functions
    ${APP_ROOT}/apps/mysql/percona_2/scripts/start_client.sh ${ip} \
            ${userid} ${password} ${1} 
fi

