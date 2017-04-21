#!/bin/bash

######################################################################################
# Notes:
#  To build LevelDB
#
#####################################################################################

BUILD_DIR="/tmp/builddir_leveldb"
VERSION="v1.20"

####################################################################################
if [ ! -f ${BUILD_DIR}/leveldb/out-shared/libleveldb.so ]; then
    echo "Leveldb has been installed successfully"
    exit 0
fi

####################################################################################
# Prepare for install
####################################################################################

mkdir -p ${BUILD_DIR}
pushd ${BUILD_DIR} > /dev/null

export CC=/usr/local/bin/gcc
export CXX=/usr/local/bin/g++

git clone https://github.com/google/leveldb
cd leveldb
git checkout --f ${VERSION}

make

if [ -d /usr/include/leveldb ] ; then
    sudo rm -fr /usr/include/leveldb
fi

sudo cp -fr ./include/* /usr/include/
sudo cp -fr ./out-shared/libleveldb* /usr/lib64/
sudo cp -fr ./out-static/libleveldb.a /usr/lib64/

popd > /dev/null

##########################################################################################
if [ -f  "/usr/include/leveldb/db.h" ]; then
    echo "Build and install leveldb successfully"
else 
    echo "Fail to build leveldb"
    exit 1
fi
