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

if [ $# -lt 1 ]; then
    echo "setup.sh {client | server}"
    exit 0
fi

#call setup utility to setup mysql 
${APP_ROOT}/toolset/setup/setup.sh "cpu" "spec_cpu2006" "setup_config.json" ${1}

