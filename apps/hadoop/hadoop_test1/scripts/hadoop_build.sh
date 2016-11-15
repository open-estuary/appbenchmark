#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To install Hadoop
#
#####################################################################################
BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/bin/hadoop)"  == 0 ]; then
      echo "Hadoop has been built successfully"
      exit 0
fi

####################################################################################
# Prepare for install
####################################################################################
rm -fr ${BUILD_DIR}
mkdir -p ${BUILD_DIR}
tar -zxvf ${SERVER_FILENAME} -C ${BUILD_DIR}
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

if [ -z "$(grep MAVEN_OPTS /etc/profile)" ] ; then 
    echo 'export MAVEN_OPTS="-Xms512m -Xmx2000m"' >> /etc/profile
fi
echo "Finish build preparation......"

######################################################################################
# Build Hadoop
######################################################################################
pushd ${BUILD_DIR}/${TARGET_DIR} > /dev/null
source /etc/profile
set Platform=aarch64
mvn package -Pdist,native,docs -DskipTests -Dtar 
popd > /dev/null

