#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
FILENAME=application-search.yml
TARNAME=micro-service-1.0-search.tar.gz
JARNAME=search.jar

IP_ARR=`ip addr | grep "eth0" | grep "inet" | awk '{ print $2}'`
IP=${IP_ARR%/*}

sudo wget -O /etc/yum.repos.d/estuary.repo https://raw.githubusercontent.com/open-estuary/distro-repo/master/estuaryftp.repo     
sudo chmod +r /etc/yum.repos.d/estuary.repo               
sudo rpm --import ftp://repoftp:repopushez7411@117.78.41.188/releases/ESTUARY-GPG-KEY               
yum clean dbcache

yum install -y micro-service-search.aarch64
mv /usr/bin/jarfile/search/ ${CUR_DIR}
pushd ${CUR_DIR}/search  > /dev/null
tar -zxvf ${TARNAME}
mv ${CUR_DIR}/search/target/htsat-search.jar ${CUR_DIR}/search/${JARNAME}
rm -rf ${CUR_DIR}/search/target ${CUR_DIR}/search/${TARNAME}
popd > /dev/null

if [ "$IP" = "192.168.1.175" ] ; then
	nohup taskset -c 48-63 java -jar -Dspring.config.location=${CUR_DIR}/${FILENAME} ${CUR_DIR}/search/${JARNAME} >/dev/null 2>&1 &
fi

if [ "$IP" = "192.168.1.160" ] ; then
	nohup taskset -c 34-46 java -jar -Dspring.config.location=${CUR_DIR}/${FILENAME} ${CUR_DIR}/search/${JARNAME} >/dev/null 2>&1 &
fi
