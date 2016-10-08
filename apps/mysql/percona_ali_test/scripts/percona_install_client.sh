#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build and install Percona-Server 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
SUBBUILD_DIR="armbuild"

if [ "${TARGET_DIR}" ] && [ "$(tool_check_exists ${LIB_INSTALL_DIR}/lib/libmysqlclient.so)"  == 0 ]; then
    echo "Target:${TARGET_DIR}"
    echo "Percona libs has been built successfully"
    exit 0
fi

echo ""
echo "##############################################################################"
echo ""
echo "Begin to install Percona libs......"

LIB_INSTALL_DIR="/u01/my3306"
pushd ${BUILD_DIR}/${TARGET_DIR}/${SUBBUILD_DIR} > /dev/null
$(tool_add_sudo) make install

$(tool_add_sudo) mkdir -p ${LIB_INSTALL_DIR}
$(tool_add_sudo) ln -s ${LIB_INSTALL_DIR}/lib/libperconsaserverclient.a  ${LIB_INSTALL_DIR}/lib/libmysqlclient.a
$(tool_add_sudo) ln -s ${LIB_INSTALL_DIR}/lib/libperconsaserverclient.so ${LIB_INSTALL_DIR}/lib/libmysqlclient.so

if [ "$(grep "${LIB_INSTALL_DIR}"  ~/.bashrc)" == "" ]; then
    $(tool_add_sudo) echo "PATH=${PATH}:${LIB_INSTALL_DIR}/bin:${LIB_INSTALL_DIR}/lib" >> ~/.bashrc
    $(tool_add_sudo) echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LIB_INSTALL_DIR}/lib" >> ~/.bashrc
fi

if [ $(tool_check_exists "/etc/ld.so.conf.d/mysql_client.conf") == 1 ]; then
    $(tool_add_sudo) touch /etc/ld.so.conf.d/mysql_client.conf
fi

$(tool_add_sudo) chmod 777 /etc/ld.so.conf.d/mysql_client.conf
$(tool_add_sudo) echo "${LIB_INSTALL_DIR}/lib" > /etc/ld.so.conf.d/mysql_client.conf
$(tool_add_sudo) ldconfig

source ~/.bashrc
popd > /dev/null


echo "Install Percona Server libs successfully"

