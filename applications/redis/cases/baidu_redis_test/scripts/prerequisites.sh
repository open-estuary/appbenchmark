#!/bin/bash

#By default, it will use apt-get to install packages
INSTALL_CMD="sudo apt-get install"
INSTALL_OPTIONS="-yq"

BUILD_ESSENTIAL="build-essential automake cmake"
COMMON_TOOLS="tcl"
#However it will use yum on other platforms such as CentOS
if [ "$(which yum)" ] ; then 
    INSTALL_CMD="sudo yum install"
    BUILD_ESSENTIAL="automake"
fi


#Add build and common tools
if [ "$(which yum)" ] ; then
    sudo yum -yq install "Devlopment Tools"
fi

${INSTALL_CMD} ${INSTALL_OPTIONS} ${BUILD_ESSENTIAL} 
${INSTALL_CMD} ${INSTALL_OPTIONS} ${COMMON_TOOLS}



