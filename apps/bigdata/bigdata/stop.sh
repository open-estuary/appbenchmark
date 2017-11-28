# !/bin/bash

CURDIR=$(cd `dirname $0`; pwd)
pushd ${CURDIR}/ansible > /dev/nul

echo "stop  bigdata"

ansible-playbook -i hosts stop.yml

popd > /dev/null

