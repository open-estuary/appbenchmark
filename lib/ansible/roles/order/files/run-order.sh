#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
FILENAME=application-order.yml
TARNAME=micro-service-1.0-order.tar.gz
JARNAME=order.jar

IP_ARR=`ip addr | grep "eth0" | grep "inet" | awk '{ print $2}'`
IP=${IP_ARR%/*}

sudo wget -O /etc/yum.repos.d/estuary.repo https://raw.githubusercontent.com/open-estuary/distro-repo/master/estuaryftp.repo     
sudo chmod +r /etc/yum.repos.d/estuary.repo
sudo rpm --import ftp://repoftp:repopushez7411@117.78.41.188/releases/ESTUARY-GPG-KEY               
yum clean dbcache

yum install -y micro-service-order.aarch64
mv /usr/bin/jarfile/order/ ${CUR_DIR}
pushd ${CUR_DIR}/order  > /dev/null
tar -zxvf ${TARNAME}
mv ${CUR_DIR}/order/target/order-0.0.1-SNAPSHOT.jar ${CUR_DIR}/order/${JARNAME}
rm -rf ${CUR_DIR}/order/target ${CUR_DIR}/order/${TARNAME}
popd > /dev/null

if [ "$IP" = "192.168.1.115" ] ; then
	nohup taskset -c 48-63 java -jar -Dspring.config.location=${CUR_DIR}/${FILENAME} ${CUR_DIR}/order/${JARNAME} 2>&1 &
fi
if [ "$IP" = "192.168.1.120" ] ; then
	nohup taskset -c 46-63 java -jar -Dspring.config.location=${CUR_DIR}/${FILENAME} ${CUR_DIR}/order/${JARNAME} 2>&1 &
fi
