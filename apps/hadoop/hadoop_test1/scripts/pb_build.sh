#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build ProtoBuffer
#
#####################################################################################

BUILD_DIR="./build_protobuffer"
CTAG_NAME="2.5.0"
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

#######################################################################################
PROTOC_VERSION="$(protoc --version)"
echo "${PROTOC_VERSION}"
if [[ "${PROTOC_VERSION}" =~ "2.5.0" ]] ; then  
      echo "ProtoBuffer-2.5.0 has been built successfully"
      exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
$(tool_add_sudo) rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR} > /dev/null
tar -zxvf ${APP_ROOT}/apps/hadoop/hadoop_test1/src/protobuf-2.5.0.tar.gz -C ./
TARGET_DIR=$(tool_get_first_dirname ./)
cp ${APP_ROOT}/apps/hadoop/hadoop_test1/src/platform_macros.h  ./${TARGET_DIR}/src/google/protobuf/stubs/platform_macros.h
cp ${APP_ROOT}/apps/hadoop/hadoop_test1/src/atomicops_internals_arm64_gcc.h  ./${TARGET_DIR}/src/google/protobuf/stubs/atomicops_internals_arm64_gcc.h
cp ${APP_ROOT}/apps/hadoop/hadoop_test1/src/Makefile.am  ./${TARGET_DIR}/src/Makefile.am
cp ${APP_ROOT}/apps/hadoop/hadoop_test1/src/atomicops.h  ./${TARGET_DIR}/src/google/protobuf/stubs/atomicops.h
echo "Finish build preparation......"

######################################################################################
# Build OpenJDK
#####################################################################################
#Build Step 1: auto generation
cd ${TARGET_DIR}
$(tool_add_sudo) chmod 755 ./configure
./configure --prefix=/usr
make -j32
$(tool_add_sudo) make install
popd > /dev/null
##########################################################################################
