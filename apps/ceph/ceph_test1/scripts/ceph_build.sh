#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build Ceph
#
#####################################################################################

BUILD_DIR="./builddir_ceph"
INSTALL_DIR="/usr/local"
VERSION="v12.0.1"
####################################################################################
if [ ! -f ${INSTALL_DIR}/bin/ceph ]; then
    echo "Ceph has been installed successfully"
    exit 0
fi

####################################################################################
# Prepare for install
####################################################################################
#$(tool_add_sudo) rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR} > /dev/null

export CC=/usr/local/bin/gcc
export CXX=/usr/local/bin/g++

git clone --recursive https://github.com/ceph/ceph.git
cd ceph
git checkout --f ${VERSION}
git submodule update --force --init --recursive

./install-deps.sh
if [ -z "$(grep 'mtune=generic' CMakeLists.txt)" ] ; then
    echo 'set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+crc -mtune=generic")' >> CMakeLists.txt
    echo 'set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+crc -mtune=generic")' >> CMakeLists.txt
fi
mkdir build
cd build
cmake ..
make -j 64
make install

popd > /dev/null

##########################################################################################
if [ "$(which ceph 2>/dev/null)" ]; then
    echo "Build and install ceph successfully"
else 
    echo "Fail to build ceph"
    exit 1
fi
