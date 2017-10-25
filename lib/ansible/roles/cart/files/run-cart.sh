#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
FILENAME=application-cart.yml
TARNAME=micro-service-1.0-cart.tar.gz
JARNAME=cart.jar

IP_ARR=`ip addr | grep "eth0" | grep "inet" | awk '{ print $2}'`
IP=${IP_ARR%/*}

#sudo wget -O /etc/yum.repos.d/estuary.repo https://raw.githubusercontent.com/open-estuary/distro-repo/master/estuaryftp.repo     
#sudo chmod +r /etc/yum.repos.d/estuary.repo               
#sudo rpm --import ftp://repoftp:repopushez7411@117.78.41.188/releases/ESTUARY-GPG-KEY               
#yum clean dbcache

yum install -y micro-service-cart.aarch64
mv /usr/bin/jarfile/cart/ ${CUR_DIR}
pushd ${CUR_DIR}/cart  > /dev/null
tar -zxvf ${TARNAME}
mv ${CUR_DIR}/cart/target/cart-0.0.1-SNAPSHOT.jar ${CUR_DIR}/cart/${JARNAME}
rm -rf ${CUR_DIR}/cart/target ${CUR_DIR}/cart/${TARNAME}
popd > /dev/null

if [[ "$IP" =~ "192.168.1.151" ]]
then
	nohup taskset -c 48-63 java -jar -Dspring.config.location=${CUR_DIR}/${FILENAME} ${CUR_DIR}/cart/${JARNAME} 2>&1 &
fi
if [ "$IP" = "192.168.1.160" ] ; then
	nohup taskset -c 47-63 java -jar -Dspring.config.location=${CUR_DIR}/${FILENAME} ${CUR_DIR}/cart/${JARNAME} 2>&1 &
fi
