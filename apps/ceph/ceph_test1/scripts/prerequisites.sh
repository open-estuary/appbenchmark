#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

#By default, it will use apt-get to install packages
INSTALL_CMD="$(tool_add_sudo) apt-get install"
INSTALL_OPTIONS="-y "

COMMON_ESSENTIAL="python2-pip"

#However it will use yum on other platforms such as CentOS

if [ "$(which yum 2>/dev/null)" ] ; then 
    INSTALL_CMD="$(tool_add_sudo) yum install"
fi

if [ "$(which apt-get 2>/dev/null)" ] ; then
    $(tool_add_sudo) ${INSTALL_CMD} ${INSTALL_OPTIONS} apt-utils
fi

$(tool_add_sudo) ${INSTALL_CMD} ${INSTALL_OPTIONS} ${COMMON_ESSENTIAL} 

