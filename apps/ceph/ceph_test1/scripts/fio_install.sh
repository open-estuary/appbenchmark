#!/bin/bash

INSTALL_CMD="yum install"

if [ ! -z "$(which apt-get 2>/dev/null)" ] ; then
    INSTALL_CMD="apt-get install"
fi


sudo ${INSTALL_CMD} -y fio

if [ -z "$(which fio 2>/dev/null)" ] ; then
    echo "Fail to install fio, please make sure that it has used epel, or estuary repostiory"
    exit 1
fi
