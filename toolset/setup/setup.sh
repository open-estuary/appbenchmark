#!/bin/bash
echo "************************************************************************"
echo "********    Welcome to Estuary Application Performance Test ************"
echo "********                                                    ************"
echo "************************************************************************"
echo ""
echo "Start to setup ${1} ${4} side for ${2} ...... "

if [ $# -lt 4 ]; then
    echo "Usage: ./common_setup.sh <app_name> <app_test_name> <app_jsoncfg_name> <client|server>"
    echo "Please note that the app, app_test_name, and app_jsoncfg_name should be consistent with names under APP_ROOT/applications"
    exit 0
fi

app_name=${1}
app_test_name=${2}
app_jsoncfg_name=${3}

#Include common setup utility functions
. ${APP_ROOT}/toolset/include/setup_includes.sh

PACKAGE_BUILD_DIR="${APP_ROOT}/buildir/${app_name}/cases/${app_test_name}/"

if [ "$(tool_check_exists ${PACKAGE_BUILD_DIR})" != 0 ] ; then
    mkdir -p ${PACKAGE_BUILD_DIR}
fi

if [ "$4" != "client" ] && [ "$4" != "server" ] ; then
    echo "Usage: ./setup.sh {client | server}"
    echo "Note: Please make sure ../config/setup_config.json have been provisioned"
    exit 0
fi 

###################################################################################
# Begin to install packages accordingly
# 
###################################################################################
setup_app() {
    pushd ${PACKAGE_BUILD_DIR} > /dev/null

    cp ${APP_ROOT}/applications/${app_name}/cases/${app_test_name}/scripts/* ./
    chmod 755 *

    echo "Begin to build ${app_name} under ${PACKAGE_BUILD_DIR}"
    python ${APP_ROOT}/toolset/setup/setup.py "${APP_ROOT}/applications/${app_name}/cases/${app_test_name}/config/${app_jsoncfg_name}" ${1}
    popd > /dev/null
}

setup_app ${4}

