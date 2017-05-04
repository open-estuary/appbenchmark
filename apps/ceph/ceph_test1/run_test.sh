#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
${CUR_DIR}/setup.sh client

if [ -z "${1}" ] ; then
    echo "Please input test type whose valid value is: basic, object, fs, or block"
    exit 1
fi

if [ -z "${2}" ] && [ "${1}" != "basic" ] ; then
    echo "Usage: ./run_test.sh <type> <monitor node name/ip>"
    exit 1
fi

if [ "${1}" == "basic" ] ; then
    ${CUR_DIR}/run_test_basic.sh $2
elif [ "${1}" == "block" ] ; then
    ${CUR_DIR}/run_test_block.sh $2 $3
elif [ "${1}" == "fs" ] ; then
    ${CUR_DIR}/run_test_fs.sh $2 $3
elif [ "${1}" == "object" ] ; then
    ${CUR_DIR}/run_test_object.sh $2 $3
else
    echo "Valid type is :basic, block, fs, or object"
    exit 1
fi

