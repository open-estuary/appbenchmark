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

#Include common setup utility functions
. ${APP_ROOT}/toolset/include/setup_includes.sh

PACKAGE_BUILD_DIR="${APP_ROOT}/buildir/mysql/cases/percona_ali_test/"

if [ "$(tool_check_exists ${PACKAGE_BUILD_DIR})" != 0 ] ; then
    mkdir -p ${PACKAGE_BUILD_DIR}
fi

if [ "$1" != "client" ] && [ "$1" != "server" ] ; then
    echo "Usage: ./setup.sh {client | server}"
    echo "Note: Please make sure ../config/setup_config.json have been provisioned"
    exit 0
fi 

###########################################
# Begin to install packages accordingly
# 
##########################################
mysql_setup() {
    pushd ${PACKAGE_BUILD_DIR} > /dev/null

    cp ${APP_ROOT}/applications/mysql/cases/percona_ali_test/scripts/* ./
    chmod 755 *

    echo "Begin to build MySQL under ${PACKAGE_BUILD_DIR}"
    tool_setup "${APP_ROOT}/applications/mysql/cases/percona_ali_test/config/setup_config.json" $1
    popd > /dev/null
}

mysql_setup $1

