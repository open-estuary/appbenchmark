# !/bin/sh

CURDIR=$(cd `dirname $0`; pwd)

pushd ${CURDIR}/ansible > /dev/null


ansible-playbook -i hosts cpu_test.yml  --user=root --extra-vars "root"

popd > /dev/null

