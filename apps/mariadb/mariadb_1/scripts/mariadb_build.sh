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
# Build MariaDB
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/BUILD
sh autorun.sh
cd ../

#Build Step2: call cmake to build system
mkdir ${SUBBUILD_DIR}
cd ${SUBBUILD_DIR}

INSTDIR="/usr/local/mariadb"
DATADIR="/u01/mariadb/data"

CFLAGS="-O3 -g -fno-omit-frame-pointer -fno-strict-aliasing -DNDEBUG -DDBUG_OFF"
CXXFLAGS="$CFLAGS -felide-constructors"

CMAKE_LAYOUT_OPTS="-DINSTALL_LAYOUT=RPM \
    -DINSTALL_SCRIPTDIR=bin \
    -DINSTALL_MYSQLDATADIR=var \
    -DINSTALL_SBINDIR=libexec \
    -DINSTALL_SUPPORTFILESDIR=share \
    -DINSTALL_SYSCONFDIR=etc \
    -DINSTALL_SYSCONF2DIR=etc/my.cnf.d 
    -DCMAKE_INSTALL_PREFIX=$INSTDIR \
    -DMYSQL_DATADIR=$DATADIR"
    
CMAKE_FEATURE_OPTS="-DWITH_READLINE=1 -DWITHOUT_OQGRAPH_STORAGE_ENGINE=1"
CMAKE_BUILD_OPTS="-DCMAKE_BUILD_TYPE=RelWithDebInfo"

cmake .. $CMAKE_BUILD_OPTS $CMAKE_LAYOUT_OPTS $CMAKE_FEATURE_OPTS \
         -DCMAKE_C_FLAGS_RELWITHDEBINFO="$CFLAGS" \
         -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="$CXXFLAGS"

make -j 64
sudo make install

echo "**********************************************************************************"
echo "Finish build MariaDB server successfully"
popd > /dev/null

##########################################################################################
echo "Build MariaDB successfully"
