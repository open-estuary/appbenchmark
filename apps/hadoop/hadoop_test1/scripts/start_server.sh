#!/bin/bash

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

##############################################################################
#Start load mode hadoop
start_local_hadoop() {
    #Update local configurations
    $(tool_add_sudo) cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/local-hdfs-site.xml ${HADOOP_INSTALL}/etc/hadoop/hdfs-site.xml
    $(tool_add_sudo) cp ${APP_ROOT}/apps/hadoop/hadoop_test1/config/local-core-site.xml ${HADOOP_INSTALL}/etc/hadoop/core-site.xml

    #Start Hadoop
    sed -i "s/export.*JAVA_HOME.*=.*\${JAVA_HOME}//g" ${HADOOP_INSTALL}/etc/hadoop/hadoop-env.sh
    echo "export JAVA_HOME=${JAVA_HOME}" >> ${HADOOP_INSTALL}/etc/hadoop/hadoop-env.sh
    ${HADOOP_INSTALL}/bin/hdfs namenode -format
    ${HADOOP_INSTALL}/sbin/start-dfs.sh
    ${HADOOP_INSTALL}/sbin/start-yarn.sh
}

start_local_hadoop
echo "Local Hadoop starts successfully"
