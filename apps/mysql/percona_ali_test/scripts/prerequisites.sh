#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

#By default, it will use apt-get to install packages
INSTALL_CMD="$(tool_add_sudo) apt-get install"
INSTALL_OPTIONS="-y -q"

BUILD_ESSENTIAL="build-essential automake cmake git"
COMMON_TOOLS="python curl wget unzip bison flex"
#However it will use yum on other platforms such as CentOS
if [ "$(which yum)" ] ; then 
    INSTALL_CMD="$(tool_add_sudo) yum install"
    BUILD_ESSENTIAL="automake"
fi

if [ "$(which apt-get)" ] ; then 
    ${INSTALL_CMD} ${INSTALL_OPTIONS} apt-utils
fi

#Add build and common tools
if [ "$(which yum)" ] ; then
    $(tool_add_sudo) yum install -y -q  "Devlopment Tools"
fi

${INSTALL_CMD} ${INSTALL_OPTIONS} ${BUILD_ESSENTIAL} 
${INSTALL_CMD} ${INSTALL_OPTIONS} ${COMMON_TOOLS}

#Add special package for percona-server build
CURSES_LIB="libncurses5-dev"
if [ $(which yum) ] ; then
    CURSES_LIB="ncurses-devel"
fi

${INSTALL_CMD} ${INSTALL_OPTIONS}  ${CURSES_LIB}

if [ "$(which yum)" ] ; then
    $(tool_add_sudo) yum install -y -q perl-ExtUtils-MakeMaker glibc-static
fi


