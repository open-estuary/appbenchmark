#!/bin/bash


CUR_DIR=$(cd `dirname $0`; pwd)
sudo ${CUR_DIR}/setup.sh server

${CUR_DIR}/scripts/start_server.sh 

