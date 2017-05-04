#!/bin/bash

#Define global APP_ROOT directory

CUR_DIR="$(cd `dirname $0`; pwd)"

echo "Please run this command on server side while its parameter is client node"
echo "Usage:./run_test_object_server.sh  <client_node>"

${CUR_DIR}/scripts/start_object_storage_server.sh ${1}

