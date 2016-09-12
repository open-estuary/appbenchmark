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

if [ "${TARGET_DIR}" ] && [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/${SUBBUILD_DIR}/libmysql/libperconaserverclient.a)"  == 0 ]; then
    echo "Target:${TARGET_DIR}"
    echo "Percona libs has been built successfully"
    exit 0
fi

echo ""
echo "##############################################################################"
echo ""
echo "Begin to prepare build $1 under ${BUILD_DIR}......"

####################################################################################
# Prepare for build
####################################################################################
sudo rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
tar -zxvf ${SERVER_FILENAME} -C ${BUILD_DIR}

echo "Try to get $(pwd) under ${BUILD_DIR}"

TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
echo "Targedir:${TARGET_DIR}"
echo "Finish build preparation......"


if [ -z ${TARGET_DIR} ] ; then
    echo "Fail to get the target directory name for $1 under ${BUILD_DIR} "
    exit 1
fi

######################################################################################
# Build percona-server 
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/BUILD
sh autorun.sh
cd ../

#Build Step 2: add bug fix for arm64 platform
if [ x$(uname -m) == x"aarch64" ] ; then
    echo "Apply bug fix for arm64 platform ......"

    sudo cp ${APP_ROOT}/applications/mysql/cases/percona_ali_test/bugfix/storage/innobase/include/sync0sync.ic \
        ./storage/innobase/include/sync0sync.ic
fi

#Build Step3: call cmake to build system
mkdir ${SUBBUILD_DIR}

LIB_INSTALL_DIR="/u01/my3306"

cd ${SUBBUILD_DIR}
cmake .. -DCMAKE_INSTALL_PREFIX=/u01/my3306 \
         -DMYSQL_DATADIR=/u01/my3306/data \
         -DMYSQL_USER=mysql  \
         -DSYSCONFDIR=/etc \
         -DWITH_MYISAM_STORAGE_ENGINE=1 \
         -DWITH_INNOBASE_STORAGE_ENGINE=1 -DWITH_MEMORY_STORAGE_ENGINE=1 \
         -DMYSQL_UNIX_ADDR=/u01/my3306/run/mysql.sock -DMYSQL_TCP_PORT=3306 \
         -DENABLED_LOCAL_INFILE=1 -DWITH_PARTITION_STORAGE_ENGINE=1 \
         -DEXTRA_CHARSETS=all \
         -DDEFAULT_CHARSET=utf8 \
         -DDEFAULT_COLLATION=utf8_general_ci

sudo make -j 32

sudo make install

echo "**********************************************************************************"
echo "Finish build percona server successfully"
popd > /dev/null

######################################################################################################################
# Begin to install percona libs
######################################################################################################################
pushd ${BUILD_DIR}/${TARGET_DIR} > /dev/null

sudo mkdir -p ${LIB_INSTALL_DIR}
sudo ln -s ${LIB_INSTALL_DIR}/lib/libperconsaserverclient.a  ${LIB_INSTALL_DIR}/lib/libmysqlclient.a
sudo ln -s ${LIB_INSTALL_DIR}/lib/libperconsaserverclient.so ${LIB_INSTALL_DIR}/lib/libmysqlclient.so

if [ "$(grep "${LIB_INSTALL_DIR}"  ~/.bashrc)" == "" ]; then
    sudo echo "PATH=${PATH}:${LIB_INSTALL_DIR}/bin:${LIB_INSTALL_DIR}/lib" >> ~/.bashrc
    sudo echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LIB_INSTALL_DIR}/lib" >> ~/.bashrc
fi

if [ $(tool_check_exists "/etc/ld.so.conf.d/mysql_client.conf") == 1 ]; then
    sudo touch /etc/ld.so.conf.d/mysql_client.conf
fi

sudo chmod 777 /etc/ld.so.conf.d/mysql_client.conf
sudo echo "${LIB_INSTALL_DIR}/lib" > /etc/ld.so.conf.d/mysql_client.conf
sudo ldconfig

source ~/.bashrc
popd > /dev/null


echo "Install Percona Server libs successfully"

