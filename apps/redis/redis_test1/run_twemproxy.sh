#!/bin/bash

##############################
# Notes:
# run twemproxy
#############################

echo "begin to run twemproxy"

TWEMPROXY_INSTALL_DIR=/usr/local/sbin

pushd ${TWEMPROXY_INSTALL_DIR} > /dev/null

taskset -c 30 ${TWEMPROXY_INSTALL_DIR}/nutcracker -d -c conf/nutcracker.yml -v 4 -m 1024 -s 22222

popd > /dev/null
