#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build OpenJdk
#
#####################################################################################

BUILD_DIR="./build_openjdk"
CTAG_NAME="112"
BRANCH_NAME="jdk8u"
TARGET_DIR=${BRANCH_NAME}${CTAG_NAME}

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/build/linux-aarch64-normal-server-release/images/j2sdk-image/bin/java)"  == 0 ]; then
      echo "Openjdk has been built successfully"
      exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
$(tool_add_sudo) rm -fr ${BUILD_DIR}
rm -r ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR} > /dev/null

if [ -z "$(which hg)" ] ; then
    echo "Please install hg tools firstly"
    exit -1
fi

hg clone http://hg.openjdk.java.net/aarch64-port/${BRANCH_NAME} ${TARGET_DIR}
cd ${TARGET_DIR}
sh get_source.sh

#####################################################################################
echo "Prepare system packages to build openjdk......"
if [ "$(which yum 2>/dev/null)" ] ; then
    $(tool_add_sudo) yum-builddep -y java-1.7.0-openjdk
    $(tool_add_sudo) yum install -y java-1.7.0-openjdk
    $(tool_add_sudo) yum install -y java-1.7.0-openjdk-devel
    $(tool_add_sudo) yum install -y cups cups-devel
    $(tool_add_sudo) yum install -y alsa-lib alsa-lib-devel libfreetype6-dev freetype-devel
    $(tool_add_sudo) yum install -y libXi libXi-devel
    $(tool_add_sudo) yum install -y libXtst-devel libXt-devel libXrender-devel
else 
    $(tool_add_sudo) apt-get install -y openjdk-7-jdk
    $(tool_add_sudo) apt-get install -y openjdk-7-jre 
    $(tool_add_sudo) apt-get install -y cups cups-common libcups2-dev
    $(tool_add_sudo) apt-get install -y libfreetype6-dev
    $(tool_add_sudo) apt-get install -y alsa-tools alsa-utils libfreetype6
    $(tool_add_sudo) apt-get install -y libxi-dev libxtst-dev libxt-dev libxrender-dev
    $(tool_add_sudo) apt-get install -y zip unzip libasound2-dev
fi

export LANG=c
export PATH="/usr/lib/jvm/java-openjdk/bin:$PATH"

echo "Finish build preparation......"

######################################################################################
# Build OpenJDK
#####################################################################################
#Build Step 1: auto generation
$(tool_add_sudo) chmod 755 ./configure
./configure  --prefix=/usr/local/openjdk
make all
popd > /dev/null
##########################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/build/linux-aarch64-normal-server-release/images/j2sdk-image/bin/java)"  == 0 ]; then
    echo "Build Openjdk successfully"
else 
    echo "Fail to build OpenJdk"
    exit 1
fi
