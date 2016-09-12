#!/bin/bash


#Include basic commands 
. ${APP_ROOT}/toolset/setup/basic_cmd.sh

##########################################################################################
# Purpose:
#   To setup application performance test environment according to the specified json file
# Usage:
#   tool_setup <json_file> {client | server}
##########################################################################################
tool_setup() {
    (
    json_file=$1
    install_elem=$2
    echo "call setup.py to install $2 based on file $1"
    python ${APP_ROOT}/toolset/setup/setup.py ${json_file} ${install_elem}
    )
}
