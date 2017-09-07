#!/bin/bash

CURDIR=$(cd `dirname $0`; pwd)
REMOTE_SERVER_ADDR=192.168.1.107

pushd ${CURDIR}/ansible > /dev/null

if [ $# -lt 1 ]; then
  echo "Usage ./setup.sh <BoardServer2UserName> <BoardServer2Ip,default for 192.168.1.107>"
  exit 1
fi

if [ $# -gt 2 ]; then
  echo "Usage ./setup.sh <BoardServer2UserName> <BoardServer2Ip,default for 192.168.1.107>"
  exit 1
fi

if [ $# -eq 2 ]; then
  echo "RemoteServerAddr ip is changed to $2"
  REMOTE_SERVER_ADDR=$2
fi

BoardServerUserName=$1

$CURDIR/get_cpu_test_tool.sh $BoardServerUserName $REMOTE_SERVER_ADDR
if [ $? -ne 0 ]; then
  exit 1
fi

ansible-playbook -i hosts site.yml  --user=root --extra-vars "root"

popd > /dev/null

