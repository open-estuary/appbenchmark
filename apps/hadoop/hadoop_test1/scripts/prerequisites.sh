#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

exit 0

#By default, it will use apt-get to install packages
INSTALL_CMD="$(tool_add_sudo) apt-get install"
INSTALL_OPTIONS="-y -q"

BUILD_ESSENTIAL="build-essential automake cmake curl mercurial maven cpio"
COMMON_TOOLS="tcl"
#However it will use yum on other platforms such as CentOS
if [ "$(which yum 2>/dev/null)" ] ; then 
    INSTALL_CMD="$(tool_add_sudo) yum install"
    BUILD_ESSENTIAL="automake"
fi

if [ "$(which apt-get 2>/dev/null)" ] ; then
    ${INSTALL_CMD} ${INSTALL_OPTIONS} apt-utils
fi

#Add build and common tools
if [ "$(which yum 2>/dev/null)" ] ; then
    $(tool_add_sudo) yum -y -q install "Devlopment Tools"
fi

${INSTALL_CMD} ${INSTALL_OPTIONS} ${BUILD_ESSENTIAL} 
${INSTALL_CMD} ${INSTALL_OPTIONS} ${COMMON_TOOLS}


if [ "$(which yum 2>/dev/null)" ] ; then
    $(tool_add_sudo) yum-builddep -y java-1.7.0-openjdk
    $(tool_add_sudo) yum install -y java-1.7.0-openjdk
    $(tool_add_sudo) yum install -y java-1.7.0-openjdk-devel
    $(tool_add_sudo) yum install -y cups cups-devel
    $(tool_add_sudo) yum install -y alsa-lib alsa-lib-devel libfreetype6-dev freetype-devel
    $(tool_add_sudo) yum install -y libXi libXi-devel zip unzip
    $(tool_add_sudo) yum install -y libXtst-devel libXt-devel libXrender-devel
else 
    $(tool_add_sudo) apt-get install -y openjdk-7-jdk
    $(tool_add_sudo) apt-get install -y openjdk-7-jre 
    $(tool_add_sudo) apt-get install -y cups cups-common libcups2-dev
    $(tool_add_sudo) apt-get install -y libfreetype6-dev
    $(tool_add_sudo) apt-get install -y alsa-tools alsa-utils libfreetype6
    $(tool_add_sudo) apt-get install -y libxi-dev libxtst-dev libxt-dev libxrender-dev
    $(tool_add_sudo) apt-get install -y zip unzip libasound2-dev
fi

