#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build and install MariaDB-Server 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
SUBBUILD_DIR="armbuild"
LIB_INSTALL_DIR="/usr/local/mariadb"

if [ "$(tool_check_exists ${LIB_INSTALL_DIR}/lib/libmysqlclient.so)"  == 0 ]; then
    echo "MariaDB libs has been installed successfully"
    exit 0
fi

echo ""
echo "##############################################################################"
echo ""
echo "Begin to install MariaDB libs......"

LIB_INSTALL_DIR="/usr/local/mariadb"
pushd ${BUILD_DIR}/${TARGET_DIR}/${SUBBUILD_DIR} > /dev/null

BASE_DIR="$(cd ~; pwd)"
if [ "$(grep "${LIB_INSTALL_DIR}"  ${BASE_DIR}/.bashrc)" == "" ]; then
    $(tool_add_sudo) echo "PATH=${PATH}:${LIB_INSTALL_DIR}/bin:${LIB_INSTALL_DIR}/lib" >> ${BASE_DIR}/.bashrc
    $(tool_add_sudo) echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LIB_INSTALL_DIR}/lib" >> ${BASE_DIR}/.bashrc
fi

if [ $(tool_check_exists "/etc/ld.so.conf.d/mysql_client.conf") == 1 ]; then
    $(tool_add_sudo) touch /etc/ld.so.conf.d/mysql_client.conf
fi

if [ -z "$(grep ${LIB_INSTALL_DIR}/lib '/etc/ld.so.conf.d/mysql_client.conf')" ] ; then
    $(tool_add_sudo) chmod 777 /etc/ld.so.conf.d/mysql_client.conf
    $(tool_add_sudo) echo "${LIB_INSTALL_DIR}/lib" > /etc/ld.so.conf.d/mysql_client.conf
fi

$(tool_add_sudo) ldconfig

source ${BASE_DIR}/.bashrc
popd > /dev/null

echo "Install MariaDB Server libs successfully"

