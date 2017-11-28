# !/bin/bash

#SOFTWARE_INSTALL_PATH=/home/BigData
echo "CurrPath=`pwd`"
CUR_DIR=$(cd `dirname $0`; pwd)

source $CUR_DIR/../common/BigDataConfig

export_tmp_envir_vars(){
    export hadoop_tmp_dir=${hadoop_tmp_dir}
    export dfs_namenode_name_dir=${dfs_namenode_name_dir}
    export dfs_datanode_data_dir=${dfs_datanode_data_dir}
}

echo "1, download hadoop$HADOOP_VERSION"
if [ ! -f ${CUR_DIR}/hadoop-${HADOOP_VERSION}.tar.gz ] ; then
 sudo wget http://mirror.bit.edu.cn/apache/hadoop/common/hadoop-${HADOOP_VERSION}/hadoop-${HADOOP_VERSION}.tar.gz -P ${CUR_DIR}
 if [ $? -ne 0 ]; then
   exit 1
 fi
fi


echo "2, decompress hadoop package"
tar -zxvf ${CUR_DIR}/hadoop-${HADOOP_VERSION}.tar.gz -C ./
if [ $? -ne 0 ]; then
  exit 1
fi

echo "3, start config hadoop$HADOOP_VERSION "


echo "3.2 start config hadoop-env.sh file"
#echo -e "export HADOOP_COMMON_HOME=\${SOFTWARE_INSTALL_PATH}/\hadoop-${HADOOP_VERSION}" >> $CUR_DIR/hadoop-${HADOOP_VERSION}/etc/hadoop/hadoop-env.sh
#export HADOOP_PREFIX=/home/dingwei/hadoop-2.7.4
echo -e "export JAVA_HOME=${SOFTWARE_INSTALL_PATH}/jdk${JAVA_VERSION}" >> ./hadoop-${HADOOP_VERSION}/etc/hadoop/hadoop-env.sh
#echo -e "export HADOOP_PREFIX=/home/dingwei/hadoop-2.7."

echo "3.3 start config yarn-env.sh file"
sed -i '/# export JAVA_HOME/a\export JAVA_HOME='"${SOFTWARE_INSTALL_PATH}"'/jdk'"${JAVA_VERSION}"'' ./hadoop-${HADOOP_VERSION}/etc/hadoop/yarn-env.sh
if [ $? -ne 0 ]; then
  exit 1
fi


echo "3.4 start config xml file"
export_tmp_envir_vars
python ${CUR_DIR}/updatexml.py ./hadoop-${HADOOP_VERSION}
if [ $? -ne 0 ]; then
  exit 1
fi

echo "4 packet the configured dir for tgz"
tar -czvf ${CUR_DIR}/../../roles/DeployBigData/files/hadoop-${HADOOP_VERSION}.tar.gz  hadoop-${HADOOP_VERSION}/
if [ $? -ne 0 ]; then
  exit 1
fi

echo "5 clear tmp files"
#rm -f ${CUR_DIR}/hadoop-${HADOOP_VERSION}.tar.gz
rm -rf hadoop-${HADOOP_VERSION}/
if [ $? -ne 0 ]; then
  exit 1
fi



