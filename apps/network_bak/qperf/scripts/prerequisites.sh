#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

#By default, it will use apt-get to install packages
INSTALL_CMD="$(tool_add_sudo) apt-get install"
INSTALL_OPTIONS="-y -q"

COMMON_TOOLS="qperf"
#However it will use yum on other platforms such as CentOS
if [ "$(which yum 2>/dev/null)" ] ; then 
    INSTALL_CMD="$(tool_add_sudo) yum install"
fi

${INSTALL_CMD} ${INSTALL_OPTIONS} ${COMMON_TOOLS}

