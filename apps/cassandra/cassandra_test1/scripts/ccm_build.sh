#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build Cassandra Cluster Management
#
#####################################################################################

BUILD_DIR="./builddir_ccm"
#INSTALL_DIR="/usr/local/cassandra"
VERSION="ccm-2.1.3"
####################################################################################
if [ "$(which ccm 2>/dev/null)" ]; then
    echo "Cassandra has been installed successfully"
    exit 0
fi

####################################################################################
# Prepare for install
####################################################################################
$(tool_add_sudo) rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR} > /dev/null

git clone https://github.com/pcmanus/ccm.git
cd ccm
git checkout --f ${VERSION}

python setup.py install

popd > /dev/null

##########################################################################################
if [ "$(which ccm 2>/dev/null)" ]; then
    echo "Build and install cassandra successfully"
else 
    echo "Fail to build cassandra"
    exit 1
fi
