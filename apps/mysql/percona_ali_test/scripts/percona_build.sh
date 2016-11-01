#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build Percona-Server 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
SUBBUILD_DIR="armbuild"

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/${SUBBUILD_DIR}/sql/mysqld)"  == 0 ]; then
      echo "Percona has been built successfully"
      exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
$(tool_add_sudo) rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
tar -zxvf ${SERVER_FILENAME} -C ${BUILD_DIR}
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

echo "Finish build preparation......"

######################################################################################
# Build percona
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/BUILD
sh autorun.sh
cd ../

#Build Step 2: add bug fix for arm64 platform
if [ x$(uname -m) == x"aarch64" ] ; then
    echo "Apply bug fix for arm64 platform ......"

#    $(tool_add_sudo) cp ${APP_ROOT}/apps/mysql/percona_ali_test/bugfix/storage/innobase/include/sync0sync.ic \
#        ./storage/innobase/include/sync0sync.ic
fi

#Build Step3: call cmake to build system
mkdir ${SUBBUILD_DIR}
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

$(tool_add_sudo) make -j 32

echo "**********************************************************************************"
echo "Finish build percona server successfully"
popd > /dev/null

##########################################################################################
echo "Build percona successfully"
