#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
FILENAME=application-order.yml
TARNAME=micro-service-1.0-order.tar.gz
JARNAME=order.jar

CPU_NUMBER=${1}

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

nohup taskset -c ${CPU_NUMBER} java -jar -Dspring.config.location=${CUR_DIR}/${FILENAME} ${CUR_DIR}/order/${JARNAME} 2>&1 &
