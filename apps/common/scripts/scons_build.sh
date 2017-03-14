#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build Scons-Server 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

export PATH=/usr/local/scons/bin:$PATH

#######################################################################################
if [ ! -z "$(which scons 2>/dev/null)" ]; then
      echo "Scons has been built successfully"
      exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
$(tool_add_sudo) rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
tar -zxvf ${SERVER_FILENAME} -C ${BUILD_DIR}
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

echo "Finish build preparation......"

######################################################################################
# Build scons
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}

python bootstrap.py build/scons
cd build/scons
python setup.py install --prefix=/usr/local/scons

if [ -z "$(grep ${INSTALL_DIR} /etc/profile)" ] ; then
    sudo sed -i '$ a export PATH=$PATH:/usr/local/scons/bin' /etc/profile
fi

##########################################################################################
echo "Build and install SCons successfully"
