#!/bin/bash

TIME_IN_SECS=300
if [ ! -z "${1}" ] ; then
    TIME_IN_SECS="${1}"
fi

CURDIR=$(cd `dirname $0`; pwd)

pushd ${CURDIR}/ansible > /dev/null

ansible-playbook -i hosts site_test.yml  --user=estuaryapp --extra-vars "ansible_sudo_pass=estuaryapp"  --extra-vars "e_commerce_test_time_insecs=${TIME_IN_SECS}"

popd > /dev/null

