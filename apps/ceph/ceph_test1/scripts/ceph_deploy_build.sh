#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build Ceph Deploy
#
#####################################################################################

BUILD_DIR="./builddir_cephdeploy"
INSTALL_DIR="/usr/local"
VERSION="v1.5.37"
####################################################################################
if [ ! -f ${INSTALL_DIR}/bin/ceph-deploy ]; then
    echo "Ceph-Deploy has been installed successfully"
    exit 0
fi

pip install ceph-deploy

##########################################################################################
if [ "$(which ceph-deploy 2>/dev/null)" ]; then
    echo "Build and install ceph-deploy successfully"
else 
    echo "Fail to build ceph"
    exit 1
fi
