# !/bin/bash

CURDIR=$(cd `dirname $0`; pwd)
pushd ${CURDIR}/ansible > /dev/nul

echo "start bigdata"

ansible-playbook -i hosts start.yml

popd > /dev/null

