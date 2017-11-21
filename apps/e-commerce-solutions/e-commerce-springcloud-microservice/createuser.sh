#!/bin/bash

CURDIR=$(cd `dirname $0`; pwd)

LOGIN_USER=${1}

if [ -z "${1}" ] ; then
  echo "Usage: createuser.sh <login username>  <new_account>"
  exit 0
fi

CREATE_USER="estuaryapp"
if [ ! -z "${2}" ] ; then
  CREATE_USER="${2}"
fi

pushd ${CURDIR}/ansible > /dev/null
echo "Please input login password for account:${LOGIN_USER}, and will create new account:${CREATE_USER}"
ansible-playbook -i hosts site_createuser.yml --user "${LOGIN_USER}" -k --extra-vars "remote_account=${CREATE_USER}"

popd > /dev/null

