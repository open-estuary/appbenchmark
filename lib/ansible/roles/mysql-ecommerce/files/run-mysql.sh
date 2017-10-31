#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
CPU_NUMBER1=${1}
CPU_NUMBER2=${2}
WORKSPACE=${3}

#sudo wget -O /etc/yum.repos.d/estuary.repo https://raw.githubusercontent.com/open-estuary/distro-repo/master/estuaryftp.repo     
#sudo chmod +r /etc/yum.repos.d/estuary.repo
#sudo rpm --import ftp://repoftp:repopushez7411@117.78.41.188/releases/ESTUARY-GPG-KEY               
#yum clean dbcache

yum install -y mysql-server

mkdir -p ${WORKSPACE}/mysql1/data
mkdir -p ${WORKSPACE}/mysql1/binlog
mkdir -p ${WORKSPACE}/mysql1/relay_log
mkdir -p ${WORKSPACE}/mysql2/data
mkdir -p ${WORKSPACE}/mysql2/binlog
mkdir -p ${WORKSPACE}/mysql2/relay_log

chown -R mysql.mysql ${WORKSPACE}/mysql1
chown -R mysql.mysql ${WORKSPACE}/mysql2

mkfs.ext4 /dev/nvme0n1p1

mount -t ext4 /dev/nvme0n1p1 ${WORKSPACE}/mysql1/data
mount -t ext4 /dev/nvme0n1p1 ${WORKSPACE}/mysql1/binlog
mount -t ext4 /dev/nvme0n1p1 ${WORKSPACE}/mysql1/relay_log
mount -t ext4 /dev/nvme0n1p1 ${WORKSPACE}/mysql2/data
mount -t ext4 /dev/nvme0n1p1 ${WORKSPACE}/mysql2/binlog
mount -t ext4 /dev/nvme0n1p1 ${WORKSPACE}/mysql2/relay_log

mkdir -p ${WORKSPACE}/mysql1/data/data1
mkdir -p ${WORKSPACE}/mysql1/data/data2
mkdir -p ${WORKSPACE}/mysql1/data/binlog1
mkdir -p ${WORKSPACE}/mysql1/data/binlog2
mkdir -p ${WORKSPACE}/mysql1/data/relay_log1
mkdir -p ${WORKSPACE}/mysql1/data/relay_log2

chown -R mysql.mysql ${WORKSPACE}/mysql1/data

mkdir -p /usr/local/mysql1
mkdir -p /usr/local/mysql2

chown -R mysql.mysql /usr/local/mysql1
chown -R mysql.mysql /usr/local/mysql2

mysql_install_db --user=mysql --datadir=${WORKSPACE}/mysql1/data/data1
mysql_install_db --user=mysql --datadir=${WORKSPACE}/mysql2/data/data2

mkdir -p /home/mysql/mydata/log
pushd /home/mysql/mydata/log > /dev/null
touch mysqld_multi.log
popd > /dev/null

taskset -c ${CPU_NUMBER1} mysqld_multi --defaults-extra-file=/etc/my.cnf start 1
taskset -c ${CPU_NUMBER2} mysqld_multi --defaults-extra-file=/etc/my.cnf start 2
