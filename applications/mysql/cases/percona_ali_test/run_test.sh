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


ip="192.168.1.187"
userid="root"
password="123456"

#Include common setup utility functions
${APP_ROOT}/applications/mysql/cases/percona_ali_test/scripts/client_start.sh ${ip} \
    ${userid} ${password} ${1}
