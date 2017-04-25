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

ESTUARY_REPO="/etc/yum.repos.d/estuary.repo"
####################################################################################
if [ ! -f "${ESTUARY_REPO}" ] ; then
    echo "Please install /etc/yum.repo.d/estuary.repo firstly before using Ceph" 
    exit 1
fi

if [ -z "$(grep "priority=1" "${ESTUARY_REPO}")" ] ; then
    echo "Make sure that estuary.repo has the highest priority(priority=1)"
    exit 1
fi

yum install ceph-deploy

##########################################################################################
if [ "$(which ceph-deploy 2>/dev/null)" ]; then
    echo "Build and install ceph-deploy successfully"
else 
    echo "Fail to build ceph"
    exit 1
fi
