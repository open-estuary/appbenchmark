#!/bin/bash

CURDIR=$(cd `dirname $0`; pwd)

pushd ${CURDIR}/ansible > /dev/null

ansible-playbook -i hosts site_remove.yml  --user=estuaryapp --extra-vars "ansible_sudo_pass=estuaryapp"  --extra-vars "packages_state=absent" --extra-vars "rpm_packages_list=Percona-Server-server-56,twemproxy,nginx,solr,micro-service-api,micro-service-cart,micro-service-discovery,micro-service-order,micro-service-search,redis,mysql"

popd > /dev/null

