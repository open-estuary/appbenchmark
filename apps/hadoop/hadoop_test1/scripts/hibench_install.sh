#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To Install HiBench
#
#####################################################################################

BUILD_DIR="./build_hibench"
CTAG_VERSION="HiBench-5.0"
TARGET_DIR="HiBench"

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/bin)"  != 0 ]; then
      echo "Hibench has not been built"
      exit 0
fi

HIBENCH_INSTALL_DIR="/usr/local/hibench"
$(tool_add_sudo) mkdir -p ${HIBENCH_INSTALL_DIR}
$(tool_add_sudo) cp -fr ${BUILD_DIR}/${TARGET_DIR}/. ${HIBENCH_INSTALL_DIR}/

#Update Hadoop Config
${tool_add_sudo} cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/hibench_hadoop.conf ${HIBENCH_INSTALL_DIR}/workloads/bayes/conf/10-baytes-userdefine.conf
${tool_add_sudo} cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/hibench_hadoop.conf ${HIBENCH_INSTALL_DIR}/workloads/join/conf/10-join-userdefine.conf
${tool_add_sudo} cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/hibench_hadoop.conf ${HIBENCH_INSTALL_DIR}/workloads/aggregation/conf/10-aggregation-userdefine.conf
${tool_add_sudo} cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/hibench_hadoop.conf ${HIBENCH_INSTALL_DIR}/workloads/pagerank/conf/10-pagerank-userdefine.conf
${tool_add_sudo} cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/hibench_hadoop.conf ${HIBENCH_INSTALL_DIR}/workloads/sleep/conf/10-sleep-userdefine.conf
${tool_add_sudo} cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/hibench_hadoop.conf ${HIBENCH_INSTALL_DIR}/workloads/scan/conf/10-scan-userdefine.conf
${tool_add_sudo} cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/hibench_hadoop.conf ${HIBENCH_INSTALL_DIR}/workloads/dfsioe/conf/10-dfsioe-userdefine.conf
${tool_add_sudo} cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/hibench_hadoop.conf ${HIBENCH_INSTALL_DIR}/workloads/wordcount/conf/10-wordcount-userdefine.conf
${tool_add_sudo} cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/hibench_hadoop.conf ${HIBENCH_INSTALL_DIR}/workloads/terasort/conf/10-terasort-userdefine.conf
${tool_add_sudo} cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/hibench_hadoop.conf ${HIBENCH_INSTALL_DIR}/workloads/sort/conf/10-sort-userdefine.conf
${tool_add_sudo} cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/hibench_hadoop.conf ${HIBENCH_INSTALL_DIR}/workloads/kmeans/conf/10-kmeans-userdefine.conf
${tool_add_sudo} cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/hibench_hadoop.conf ${HIBENCH_INSTALL_DIR}/workloads/nutchindexing/conf/10-nuthindexing-userdefine.conf
echo "Hibench has been install under ${HIBENCH_INSTALL_DIR}"

