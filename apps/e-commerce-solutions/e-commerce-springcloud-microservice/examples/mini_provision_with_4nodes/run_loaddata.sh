#!/bin/bash

CURDIR=$(cd `dirname $0`; pwd)

pushd ${CURDIR}/ansible > /dev/null

ansible-playbook -i hosts site_loaddata.yml  --user=estuaryapp --extra-vars "ansible_sudo_pass=estuaryapp"

popd > /dev/null

