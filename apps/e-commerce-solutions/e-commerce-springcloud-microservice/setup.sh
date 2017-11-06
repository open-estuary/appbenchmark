#!/bin/bash

CURDIR=$(cd `dirname $0`; pwd)

TARGET=arm64
if [ ! -z "${1}" ] ; then
    TARGET="${1}"
fi

pushd ${CURDIR}/ansible > /dev/null

ansible-playbook -i hosts site_${TARGET}.yml  --user=estuaryapp --extra-vars "ansible_sudo_pass=estuaryapp"

popd > /dev/null

