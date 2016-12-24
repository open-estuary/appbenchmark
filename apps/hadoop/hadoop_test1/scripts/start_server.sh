#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

if [ $# -lt 1 ]; then
    echo "Usage: start_server.sh <local | remote>"
    exit 0
fi

BASE_DIR=$(cd ~; pwd)

source /etc/profile

if [ -z "${HADOOP_INSTALL}" ] ; then
    echo "HADOOP_INSTALL is not set so far. Probably Hadoop has not been installed"
    exit 0
fi

NUMA_CMD=""
#NUMA_CMD="numactl --cpunodebind=0,1,2,3 --localalloc

##############################################################################
#Start load mode hadoop
start_local_hadoop() {
    #Update local configurations
    $(tool_add_sudo) cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/local-hdfs-site.xml ${HADOOP_INSTALL}/etc/hadoop/hdfs-site.xml
    $(tool_add_sudo) cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/local-core-site.xml ${HADOOP_INSTALL}/etc/hadoop/core-site.xml
    $(tool_add_sudo) cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/local-yarn-site.xml ${HADOOP_INSTALL}/etc/hadoop/yarn-site.xml
    $(tool_add_sudo) cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/local-mapred-site.xml ${HADOOP_INSTALL}/etc/hadoop/mapred-site.xml
    #Start Hadoop
    sed -i "s/export.*JAVA_HOME.*=.*\${JAVA_HOME}//g" ${HADOOP_INSTALL}/etc/hadoop/hadoop-env.sh
    echo "export JAVA_HOME=${JAVA_HOME}" >> ${HADOOP_INSTALL}/etc/hadoop/hadoop-env.sh
    ${HADOOP_INSTALL}/bin/hdfs namenode -format
    ${NUMA_CMD} ${HADOOP_INSTALL}/sbin/start-dfs.sh
    ${NUMA_CMD} ${HADOOP_INSTALL}/sbin/start-yarn.sh
}

start_local_hadoop
echo "Local Hadoop starts successfully"
