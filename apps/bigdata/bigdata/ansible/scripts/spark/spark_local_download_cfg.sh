# !/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)

source $CUR_DIR/../common/BigDataConfig

version=${HADOOP_VERSION}
echo "1, download spark-${SPARK_VERSION}-bin-hadoop${version%.*}.tgz"
if [ ! -f ${CUR_DIR}/spark-${SPARK_VERSION}-bin-hadoop${version%.*}.tgz ] ; then
  sudo wget http://mirror.bit.edu.cn/apache/spark/spark-${SPARK_VERSION}/spark-${SPARK_VERSION}-bin-hadoop${version%.*}.tgz -P ${CUR_DIR}
 if [ $? -ne 0 ]; then
   exit 1
 fi
fi

echo "2, decompress hadoop package"
tar -zxvf ${CUR_DIR}/spark-${SPARK_VERSION}-bin-hadoop${version%.*}.tgz -C ./
if [ $? -ne 0 ]; then
  exit 1
fi


cd spark-${SPARK_VERSION}-bin-hadoop${version%.*}/conf
if [ $? -ne 0 ]; then
  exit 1
fi

echo "3, start config spark-${SPARK_VERSION}-bin-hadoop${version%.*} "

#echo "3.1 start config spark-env.sh file"
#cp spark-env.sh.template spark-env.sh
#if [ $? -ne 0 ]; then
#  exit 1
#fi

#echo -e "export SCALA_HOME=$SOFTWARE_INSTALL_PATH/scala-$SCALA_VERSION" >> spark-env.sh
#echo -e "export JAVA_HOME=$SOFTWARE_INSTALL_PATH/jdk$JAVA_VERSION" >> spark-env.sh
#echo -e "export HADOOP_HOME=$SOFTWARE_INSTALL_PATH/hadoop-$HADOOP_VERSION" >> spark-env.sh
#echo -e "export HADOOP_CONF_DIR=\$HADOOP_HOME/etc/hadoop" >> spark-env.sh
#echo -e "SPARK_MASTER_IP=centos" >> spark-env.sh
#echo -e "SPARK_LOCAL_DIRS=$SOFTWARE_INSTALL_PATH/spark-${SPARK_VERSION}-bin-hadoop${version%.*}" >> spark-env.sh
#echo "SPARK_DRIVER_MEMORY=2G" >> spark-env.sh
#if [ $? -ne 0 ]; then
#  exit 1
#fi




cd -
if [ $? -ne 0 ]; then
  exit 1
fi

echo "4 packet the configured dir for tgz"
tar -czvf ${CUR_DIR}/../../roles/DeployBigData/files/spark-${SPARK_VERSION}-bin-hadoop${version%.*}.tar.gz  spark-${SPARK_VERSION}-bin-hadoop${version%.*}/
if [ $? -ne 0 ]; then
  exit 1
fi


echo "5 cleear tmp files"
#rm -f ${CUR_DIR}/spark-${SPARK_VERSION}-bin-hadoop${version%.*}.tgz
rm -rf spark-${SPARK_VERSION}-bin-hadoop${version%.*}/
if [ $? -ne 0 ]; then
  exit 1
