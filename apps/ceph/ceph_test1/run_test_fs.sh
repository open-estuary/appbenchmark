#!/bin/bash

#Define global APP_ROOT directory

CUR_DIR="$(cd `dirname $0`; pwd)"

INSTALL_CMD="yum install"
if [ ! -z "$(which apt-get)" ] ; then
    INSTALL_CMD="apt-get install"
fi

sudo ${INSTALL_CMD} -y bonnie++




