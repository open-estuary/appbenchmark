#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
VERSION="1.6"
Mycat_FILENAME=Mycat-server-1.6-RELEASE-20161028204710-linux.tar.gz
SRC_DIR=src
WRAPPER_FILENAME=wrapper-linux-aarch64-64

if [ ! -f ${CUR_DIR}/${SRC_DIR}/${Mycat_FILENAME} ] ; then
    if [ ! -f ${CUR_DIR}/${SRC_DIR} ] ; then
        mkdir -p ${CUR_DIR}/${SRC_DIR}
    fi
    wget -O ${CUR_DIR}/${SRC_DIR}/${Mycat_FILENAME} https://github.com/MyCATApache/Mycat-download/raw/master/1.6-RELEASE/${Mycat_FILENAME}
fi

#unzip source file
pushd ${CUR_DIR}/${SRC_DIR} > /dev/null
sudo tar -zxvf ${Mycat_FILENAME}
sudo cp ${CUR_DIR}/${WRAPPER_FILENAME} ${CUR_DIR}/${SRC_DIR}/mycat/bin/
sudp cp ${CUR_DIR}/${WRAPPER_FILENAME} ${CUR_DIR}/${SRC_DIR}/mycat/bin/wrapper
sudo cp libwrapper.so /usr/lib64/

pushd  > /dev/null



