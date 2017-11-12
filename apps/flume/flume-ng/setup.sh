#!/bin/bash


CURDIR=$(cd `dirname $0`; pwd)

pushd ${CURDIR}/ansible > /dev/null

ansible-playbook -vvv -i hosts site.yml --user=root --extra-vars "ansible_sudo_pass=root"  

popd > /dev/null

