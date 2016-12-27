#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To Install HiBench
#
#####################################################################################
TARGET_FILE=${1}

#######################################################################################

SPEC_INSTALL_DIR="/usr/local/jvm/specjvm2008"

mkdir -p ${SPEC_INSTALL_DIR}

cp ${TARGET_FILE} ${SPEC_INSTALL_DIR}/

source /etc/profile
pushd ${SPEC_INSTALL_DIR} > /dev/null
${JAVA_HOME}/bin/java -jar ${TARGET_FILE} -DUSER_INSTALL_DIR=${SPEC_INSTALL_DIR} -i silent

cp ${APP_ROOT}/apps/jvm/specjvm2008/config/specjvm.properties  ${SPEC_INSTALL_DIR}/props/specjvm.properties

popd > /dev/null

echo "Specjvm2008 has been install under ${SPEC_INSTALL_DIR}"

