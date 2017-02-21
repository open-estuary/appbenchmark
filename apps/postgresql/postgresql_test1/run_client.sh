#!/bin/bash

if [ "$(whoami)" == "root" ] ; then
    echo "It must use non-root account to execute this script"
fi

CUR_DIR=$(cd `dirname $0`; pwd)
sudo ${CUR_DIR}/setup.sh client
