#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
FILENAME=${CUR_DIR}/netperf-2.4.5.tar.gz

sudo tar -zxvf ${FILENAME} -C ${CUR_DIR}/

cd ${CUR_DIR}/netperf-2.4.5/
./configure --build=aarch64-unknown-linux-gnu
make && make install

