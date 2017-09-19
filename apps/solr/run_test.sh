#!/bin/bash

CURDIR=$(cd `dirname $0`; pwd)

pushd ${CURDIR}/ansible > /dev/null

ansible-playbook -i hosts test.yml  --user=estuaryapp --extra-vars "ansible_sudo_pass=estuary"

popd > /dev/null

