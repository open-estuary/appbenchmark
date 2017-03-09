#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build tcprstat
#
#####################################################################################

BUILD_DIR="./builddir_tcprstat"
TARGET_DIR="master"

####################################################################################
# Prepare for build
####################################################################################
if [ $(tool_check_exists "/usr/sbin/tcprstat") == 0 ] ; then
    echo "tcprstat has been built, so do nothing"
    echo "Build tcprstat successfully"
    exit 0 
fi

$(tool_add_sudo) rm -fr ${BUILD_DIR}/${TARGET_DIR}*
mkdir -p ${BUILD_DIR}

git clone https://github.com/Lowercases/tcprstat.git ${BUILD_DIR}/${TARGET_DIR}

#Replace libpcap1.1 with libpcap1.6 in order to support ARM64 platform
#if [ $(uname -m) == "aarch64" ] ; then
    echo "Replace libpcap1.1 with libpcap1.6 in order to support arm64 platform"
    tool_download -o libpcap-1.6.1.tar.gz http://www.tcpdump.org/release/libpcap-1.6.1.tar.gz
    cp libpcap-1.6.1.tar.gz ${BUILD_DIR}/${TARGET_DIR}/libpcap/
    rm ${BUILD_DIR}/${TARGET_DIR}/libpcap/libpcap-1.1.1.tar.gz
    chmod 755 ${BUILD_DIR}/${TARGET_DIR}/libpcap/resolver-patch
    sed -i "s/libpcap-1.1.1/libpcap-1.6.1/g"  ${BUILD_DIR}/${TARGET_DIR}/libpcap/resolver-patch
#fi

echo "Finish build preparation......"

######################################################################################
# Build TcpRstat
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/nulli
cd ${TARGET_DIR}/

CONFIGURE_OPTIONS=""
if [ $(uname -m) == "aarch64" ] ; then
    CONFIGURE_OPTIONS=${CONFIGURE_OPTIONS}" -build=arm "

    #Add bug fix for arm platform
    $(tool_add_sudo) cp ${APP_ROOT}/apps/mysql/percona_2/bugfix/tcprstat/src/tcprstat.c \
            ./src/tcprstat.c 
fi

chmod 755 bootstrap
./bootstrap
./configure ${CONFIGURE_OPTIONS}
make

if [ $(tool_check_exists /usr/bin/tcprstat-static) == 0 ]; then
    echo "tcpstat-static has been installed "
else 
    $(tool_add_sudo) cp ./src/tcprstat /usr/sbin/tcprstat
    $(tool_add_sudo) cp ./src/tcprstat-static /usr/sbin/tcprstat-static
    $(tool_add_sudo) cp ./src/tcprstat /usr/sbin/tcprstat
fi

$(tool_add_sudo) chmod u+s /usr/sbin/tcprstat
$(tool_add_sudo) chmod u+s /usr/sbin/tcprstat-static
$(tool_add_sudo) chmod u+s /usr/sbin/tcprstat

popd > /dev/null

echo "**********************************************************************************"
echo "Build tcprstat completed"

