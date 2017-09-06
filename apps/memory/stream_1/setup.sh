#!/bin/bash

ssh-copy-id  -i /root/.ssh/id_rsa.pub  root@192.168.1.115
ssh-copy-id  -i /root/.ssh/id_rsa.pub  root@192.168.1.225
ssh-copy-id  -i /root/.ssh/id_rsa.pub  root@192.168.1.234

CURDIR=$(cd `dirname $0`; pwd)

pushd ${CURDIR}/ansible > /dev/null

ansible-playbook setup.yml

popd > /dev/null

