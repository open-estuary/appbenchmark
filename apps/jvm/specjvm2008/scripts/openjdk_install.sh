#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To install OpenJdk
#
#####################################################################################

BUILD_DIR="./build_openjdk"
CTAG_NAME="112"
BRANCH_NAME="jdk8u"

TARGET_FILE=${1}
TARGET_DIR="/usr/local/openjdk/jvm/"

####################################################################################
# To install OpenJdk
####################################################################################
#pushd ${BUILD_DIR}/${TARGET_DIR} > /dev/null
#$(tool_add_sudo) make install

echo "TAGET_FILE:${TARGET_FILE}"
mkdir -p ${TARGET_DIR}
cp ${TARGET_FILE} ${BUILD_DIR}
cd ${BUILD_DIR}
xz -d ${TARGET_FILE}
TARGET_FILE=${TARGET_FILE:0:-3}
tar -xvf ${TARGET_FILE} -C ${TARGET_DIR}
mv ${TARGET_DIR}/${TARGET_FILE:0:-4}* ${TARGET_DIR}/openjdk-1.8.0-internal
#popd > /dev/null

if [ -z "$(grep "/usr/local/openjdk" /etc/profile)" ] ; then 
    
    echo 'export JAVA_HOME=/usr/local/openjdk/jvm/openjdk-1.8.0-internal' >> /etc/profile
    echo 'export PATH=${JAVA_HOME}/bin:$PATH' >> /etc/profile
    echo 'export CLASSPATH=.:${JAVA_HOME}/lib/dt.jar:${JAVA_HOME}/lib/tools.jar' >> /etc/profile
fi

source /etc/profile

##########################################################################################
if [ "$(which java 2>/dev/null)" ]; then
    echo "Install Openjdk successfully"
else 
    echo "Fail to build OpenJdk"
    exit 1
fi

