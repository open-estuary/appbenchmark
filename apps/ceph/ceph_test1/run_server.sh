#!/bin/bash

if [ "$(whoami)" == "root" ] ; then
    echo "It must use non-root to execute these packages"
    exit 0
fi

CUR_DIR=$(cd `dirname $0`; pwd)
sudo ${CUR_DIR}/setup.sh server

${CUR_DIR}/scripts/start_server.sh

