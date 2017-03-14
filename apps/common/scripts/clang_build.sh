#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build Clang-Server 
#
#####################################################################################

BUILD_DIR="./build_clang"
SERVER_FILENAME=$1
TARGET_DIR="clang"

#######################################################################################
if [ -f "/usr/local/bin/clang" ]; then
  echo "Clang has been built successfully"
  exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
$(tool_add_sudo) rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR} > /dev/null

if [ -z "$(which cmake 2>/dev/null)" ] ; then
    wget https://cmake.org/files/v3.7/cmake-3.7.0-rc1.tar.gz
    tar -zxf cmake-*.tar.gz
    cd cmake-3.7.0-rc1  
    ./bootstrap --parallel=32 --bindir=/bin/
    make
    #Remove old cmake
    ${SUDO_PREFIX} yum remove -y cmake
    ${SUDO_PREFIX} make install
    cd ..
fi

if [ -z "$(which svn 2>/dev/null)" ] ; then
    if [ ! -z "$(whch apt-get 2>/dev/null)" ] ; then
        sudo apt-get install -y svn
    else
        sudo yum install -y svn
    fi
fi

svn co http://llvm.org/svn/llvm-project/llvm/tags/RELEASE_390/final/ llvm
#svn co http://llvm.org/svn/llvm-project/llvm/trunk/ llvm

cd ./llvm/tools
svn co http://llvm.org/svn/llvm-project/cfe/tags/RELEASE_390/final/ clang
#svn co http://llvm.org/svn/llvm-project/cfe/trunk/ clang
cd ../

#cd ../
#svn co http://llvm.org/svn/llvm-project/clang-tools-extra/trunk extra
#cd extra
#svn update

#cd ./projects
#svn co http://llvm.org/svn/llvm-project/compiler-rt/trunk compiler-rt
#cd ../

mkdir ../build_clang
cd ../build_clang
  
#cmake  ../llvm 
cmake -DCMAKE_BUILD_TYPE=Release ../llvm  
make -j64
cmake --build . --parallel=64
cmake --build . --target install

#make -j32
#make install
  
popd > /dev/null

if [ -z "$(which clang)" ] ; then
    echo "Fail to install clang"
    exit 0
fi

##########################################################################################
echo "Build clang successfully"
