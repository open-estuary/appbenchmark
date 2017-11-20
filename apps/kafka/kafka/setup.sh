#!/bin/bash

CURDIR=$(cd `dirname $0`; pwd)

pushd ${CURDIR}/ansible > /dev/null

ansible-playbook -i hosts site.yml  --user=dingwei -s

popd > /dev/null

