#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
FILENAME=${CUR_DIR}/redis-4.0.2.tar.gz

sudo tar -zxvf ${FILENAME} -C ${CUR_DIR}/

cd ${CUR_DIR}/redis-4.0.2/
make 

REDIS_INSTALL_DIR="/usr/local/redis/"

sudo mkdir -p ${REDIS_INSTALL_DIR}/bin
sudo cp src/redis-server ${REDIS_INSTALL_DIR}/bin
sudo cp src/redis-cli ${REDIS_INSTALL_DIR}/bin
sudo cp src/redis-benchmark ${REDIS_INSTALL_DIR}/bin

