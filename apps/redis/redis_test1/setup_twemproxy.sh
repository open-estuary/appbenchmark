#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
sudo git clone https://github.com/twitter/twemproxy.git
cd ${CUR_DIR}/twemproxy

sudo autoreconf -fvi
sudo ./configure --build=aarch64-unknown-linux-gnu
make && make install

TWEMPROXY_INSTALL_DIR="/usr/local/sbin"

sudo mkdir -p ${TWEMPROXY_INSTALL_DIR}/conf
sudo cp ../config/nutcracker.yml   ${TWEMPROXY_INSTALL_DIR}/conf

echo "twemproxy has been installed successfully"
