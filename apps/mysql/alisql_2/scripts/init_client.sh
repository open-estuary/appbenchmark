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


#ip="192.168.1.186"
ip=${1}
userid="root"
password="123456"

if [ $# -lt 2 ] ; then 
    echo "Usage: ./run_test.sh {ip} {init | loaddata | test} [inst_num]"
    exit 0
else 
   echo "Try to test multi mysql instances[$3]......"

   inst_num=${3}
   if [ "x${1}" == "xtest" ] ; then
      ${APP_ROOT}/apps/mysql/alisql_2/scripts/start_client.sh ${ip} \
               ${userid} ${password} ${2} ${inst_num}
   else    

       port_num=3306
       cur_inst=0
       while [[ ${cur_inst} -lt ${inst_num} ]]
       do
           ${APP_ROOT}/apps/mysql/alisql_2/scripts/start_client.sh ${ip} \
               ${userid} ${password} ${2} ${port_num} 
           let "port_num++"
           let "cur_inst++"
       done
   fi
fi

