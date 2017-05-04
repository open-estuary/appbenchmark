#!/bin/bash

if [ -z "${1}" ] ; then
    echo "Must input monitor server name"
    exit 1
fi

mon_server="${1}"

CUR_DIR=$(cd `dirname $0`; pwd)
${CUR_DIR}/scripts/clean_server.sh  ${mon_server}

