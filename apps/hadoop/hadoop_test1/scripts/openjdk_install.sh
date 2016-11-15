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
TARGET_DIR=${BRANCH_NAME}${CTAG_NAME}

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/build/linux-aarch64-normal-server-release/images/j2sdk-image/bin/java)"  != 0 ]; then
      echo "Openjdk has not been built successfully"
      exit -1
fi

####################################################################################
# To install OpenJdk
####################################################################################
pushd ${BUILD_DIR}/${TARGET_DIR} > /dev/null
$(tool_add_sudo) make install
popd > /dev/null

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

