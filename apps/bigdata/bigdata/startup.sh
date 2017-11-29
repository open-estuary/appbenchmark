# !/bin/bash

CURDIR=$(cd `dirname $0`; pwd)
pushd ${CURDIR}/ansible > /dev/nul

echo "startup bigdata"

ansible-playbook -i hosts startup.yml -vvv

popd > /dev/null

