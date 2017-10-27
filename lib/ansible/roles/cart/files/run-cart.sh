#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
FILENAME=application-cart.yml
TARNAME=micro-service-1.0-cart.tar.gz
JARNAME=cart.jar

CPU_NUMBER=${1}

sudo wget -O /etc/yum.repos.d/estuary.repo https://raw.githubusercontent.com/open-estuary/distro-repo/master/estuaryftp.repo     
sudo chmod +r /etc/yum.repos.d/estuary.repo               
sudo rpm --import ftp://repoftp:repopushez7411@117.78.41.188/releases/ESTUARY-GPG-KEY               
yum clean dbcache

yum install -y micro-service-cart.aarch64
mv /usr/bin/jarfile/cart/ ${CUR_DIR}
pushd ${CUR_DIR}/cart  > /dev/null
tar -zxvf ${TARNAME}
mv ${CUR_DIR}/cart/target/cart-0.0.1-SNAPSHOT.jar ${CUR_DIR}/cart/${JARNAME}
rm -rf ${CUR_DIR}/cart/target ${CUR_DIR}/cart/${TARNAME}
popd > /dev/null

nohup taskset -c ${CPU_NUMBER} java -jar -Dspring.config.location=${CUR_DIR}/${FILENAME} ${CUR_DIR}/cart/${JARNAME} 2>&1 &
