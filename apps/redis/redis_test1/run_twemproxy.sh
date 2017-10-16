#!/bin/bash

##############################
# Notes:
# run twemproxy
#############################

echo "begin to run twemproxy"

TWEMPROXY_INSTALL_DIR=/usr/local/sbin

pushd ${TWEMPROXY_INSTALL_DIR} > /dev/null

sudo taskset -c 30 ${TWEMPROXY_INSTALL_DIR}/nutcracker conf/nutcracker.yml -v 4 -m 1024

popd > /dev/null
