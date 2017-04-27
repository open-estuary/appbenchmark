#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
CASSANDRA_HOME=/usr/local/ceph
BASE_DIR="/u01/ceph"
OSD_DIR="${BASE_DIR}"/osd
MON_DIR="${BASE_DIR}"/mon
DEFAULT_USER="ceph"

if [ ! -d "${BASE_DIR}" ] ; then
    mkdir -p ${BASE_DIR}
    echo "Please mount /u01/ceph to different disk partition"
fi

sudo useradd ${DEFAULT_USER}

if [ ! -d "${OSD_DIR}" ] ; then
    mkdir -p ${OSD_DIR}
fi

if [ ! -d "${MON_DIR}" ] ; then
    mkdir -p ${MON_DIR}
fi
sudo chown -R ${DEFAULT_USER} ${BASE_DIR}
sudo chown -R ${DEFAULT_USER} ${OSD_DIR}
sudo chown -R ${DEFAULT_USER} ${MON_DIR}

INSTALL_CMD="yum"
if [ -z "$(which yum 2>/dev/null)" ] ; then
    INSTALL_CMD="apt-get"
fi

hostname=$(hostname)

#########################################################################################
# Prepare
#########################################################################################
sudo ${INSTALL_CMD} install -y ntp ntpupdate
sudo ${INSTALL_CMD} install openssh-server
ntpdate pool.ntp.org
systemctl restart netdate.service
systemctl restart ntpd.service
systemctl enable ntpd.service
systemctl enable ntpdate.service
service sshd restart
sudo setenforce 0

##########################################################################################
# Setup Ceph: Basic Setup
##########################################################################################
cd ${BASE_DIR}

if [ ! -f ceph.conf ] ; then
    sudo ceph-deploy new ${hostname}
    sudo ceph-deploy install ${hostname}

    sudo sed -i '/\[global\]/a mon\ data\ avail\ crit\ =\ 1' ceph.conf
    sudo sed -i '/\[global\]/a public\ network\ =\ 192\.168\.1\.0\/24' ceph.conf

    sudo sed -i '/\[global\]/a max\ open\ files\ =\ 131072' ceph.conf

    sudo sed -i '/\[global\]/a osd_max_object_name_len\ =\ 256' ceph.conf
    sudo sed -i '/\[global\]/a osd_max_object_namespace_len\ =\ 64' ceph.conf
fi

if [ -z "$(grep -r "\[mon\]")" ] ; then
    sudo cat ${CUR_DIR}/../config/mon_osd.conf >> ceph.conf
fi

#sudo -u ${DEFAULT_USER} ceph-mon --mkfs -i ${hostname} --keyring ${BASE_DIR}/ceph.mon.keyring
#sudo -u ${DEFAULT_USER} touch ${MON_DIR}/done

if [ -z "$(ps -aux | grep ceph-mon | grep -v grep 2>/dev/null)" ] ; then
    sudo ceph-deploy --overwrite-conf mon create-initial 
fi

##########################################################################################
# Setup Ceph: OSD
##########################################################################################
sudo ceph-deploy --overwrite-conf osd prepare ${hostname}:${OSD_DIR}
sudo ceph-deploy osd activate ${hostname}:${OSD_DIR}
sudo ceph-deploy admin ${hostname}
sudo chmod +r /etc/ceph/ceph.client.admin.keyring

##########################################################################################
# Check Ceph status
##########################################################################################
echo "Checking ceph cluster status ......"
ceph health

echo "Ceph has been started in /u01/ceph directory ..."
echo "**********************************************************************************"

