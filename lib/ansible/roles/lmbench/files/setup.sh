#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
FILENAME=${CUR_DIR}/lmbench.tgz

sudo tar -zxvf ${FILENAME} -C ${CUR_DIR}/

cd ${CUR_DIR}/lmbench-3.0-a9/src && make
make install
