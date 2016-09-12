#!/bin/bash
#################################################################
# Define Global Variables and Macros
#
#################################################################

if [ -z ${APP_ROOT} ]; then
    echo "APP_ROOT macro has not been defined yet"
    exit 0
fi

export PATH=${APP_ROOT}:$PATH
