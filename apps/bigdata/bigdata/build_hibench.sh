# !/bin/bash

CURDIR=$(cd `dirname $0`; pwd)
pushd ${CURDIR}/ansible > /dev/nul

ansible-playbook -i hosts build_hibench.yml -vvv

popd > /dev/null
