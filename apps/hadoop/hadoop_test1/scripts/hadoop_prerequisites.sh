#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

#By default, it will use apt-get to install packages
INSTALL_CMD="$(tool_add_sudo) apt-get install"
INSTALL_OPTIONS="-y"

COMMON_TOOLS="maven zlib1g-dev pkg-config libssl-dev protobuf-compiler"
if [ "$(which yum 2>/dev/null)" ] ; then 
    INSTALL_CMD="$(tool_add_sudo) yum install"
    COMMON_TOOLS="maven zlib1g-devel pkg-config libssl-devel protobuf-compiler"
fi

${INSTALL_CMD} ${INSTALL_OPTIONS} ${COMMON_TOOLS}



