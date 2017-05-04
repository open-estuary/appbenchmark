#!/bin/bash

SIGNLE_NODE="yes"
HOSTNAME=$(hostname)

if [ ! -z "${1}" ] ; then
    HOSTNAME="${1}"
fi

if [ ! -z "${2}" ] ; then
    SIGNLE_NODE="${2}"
fi

CUR_DIR=$(cd `dirname $0`; pwd)
CASSANDRA_HOME=/usr/local/ceph
BASE_DIR="/u01/ceph"
OSD_BASE_DIR="${BASE_DIR}"/osd
MON_DIR="${BASE_DIR}"/mon

DEFAULT_USER="ceph"
CUR_USER=`whoami`

if [ x"${SIGNLE_NODE}" == x"yes" ] ; then
    POOL_SIZE=1
else 
    POOL_SIZE=3
fi

##############################################################################################
echo "Change POOL_SIZE when it is to install ceph on multiple node!" 


#if [ "${DEFAULT_USER}" == "root" ] ; then
#    echo "Please use non-root account to setup ceph"
#    exit 0
#fi

if [ ! -d "${BASE_DIR}" ] ; then
    mkdir -p ${BASE_DIR}
    echo "Please mount /u01/ceph to different disk partition"
fi

#${CUR_DIR}/ceph_users_setup.sh ${DEFAULT_USER}  ${HOSTNAME}

ssh-keygen
ssh-copy-id ${CUR_USER}@${HOSTNAME}

for index in 1 2 3
do
    let "index--" 
    OSD_DIR="${OSD_BASE_DIR}""${index}"
    if [ ! -d "${OSD_DIR}" ] ; then
        mkdir -p ${OSD_DIR}
    fi
    sudo chown -R ${DEFAULT_USER} ${OSD_DIR}
done

if [ ! -d "${MON_DIR}" ] ; then
    mkdir -p ${MON_DIR}
fi
sudo chown -R ${DEFAULT_USER} ${BASE_DIR}
#sudo chown -R ${DEFAULT_USER} ${OSD_DIR}
sudo chown -R ${DEFAULT_USER} ${MON_DIR}

INSTALL_CMD="yum"
if [ -z "$(which yum 2>/dev/null)" ] ; then
    INSTALL_CMD="apt-get"
fi

#########################################################################################
# Prepare
#########################################################################################
sudo ${INSTALL_CMD} install -y ntp ntpupdate
sudo ${INSTALL_CMD} install openssh-server
sudo ntpdate pool.ntp.org
sudo systemctl restart netdate.service
sudo systemctl restart ntpd.service
sudo systemctl enable ntpd.service
sudo systemctl enable ntpdate.service

sudo sed -i 's/^\#RSAAuthentication\ .*/RSAAuthentication\ yes/g' /etc/ssh/sshd_config
sudo sed -i 's/^\#PubkeyAuthentication\ .*/PubkeyAuthentication\ yes/g' /etc/ssh/sshd_config

sudo service sshd restart
sudo setenforce 0

##########################################################################################
# Setup Ceph: Basic Setup
##########################################################################################
cd ${BASE_DIR}

if [ ! -f ceph.conf ] ; then
    ceph-deploy new ${HOSTNAME}
    sed -i '/\[global\]/a mon\ data\ avail\ crit\ =\ 1' ceph.conf
    sed -i '/\[global\]/a public\ network\ =\ 192\.168\.1\.0\/24' ceph.conf

    sed -i '/\[global\]/a max\ open\ files\ =\ 131072' ceph.conf

    sed -i '/\[global\]/a rbd_default_features\ =\ 1' ceph.conf
    
    sed -i '/\[global\]/a pid\ file\ =\ \/u01\/ceph\/ceph\.\$type\.\$id\.pid' ceph.conf
    sed -i '/\[global\]/a osd_max_object_name_len\ =\ 256' ceph.conf
    sed -i '/\[global\]/a osd_max_object_namespace_len\ =\ 64' ceph.conf
fi

if [ -z "$(grep -r "\[mon\]")" ] ; then
    cat ${CUR_DIR}/../config/mon_osd.conf >> ceph.conf

    if [ x"${SIGNLE_NODE}" == x"yes" ] ; then
        sed -i "/\[global\]/a osd\ pool\ default\ size\ =\ ${POOL_SIZE}" ceph.conf
        sed -i '/\[global\]/a osd\ crush\ chooseleaf\ type\ =\ 0' ceph.conf
    fi
fi

ceph-deploy install ${HOSTNAME}
#ceph-deploy --username ${DEFAULT_USER} install ${HOSTNAME}

#sudo -u ${DEFAULT_USER} ceph-mon --mkfs -i ${HOSTNAME} --keyring ${BASE_DIR}/ceph.mon.keyring
#sudo -u ${DEFAULT_USER} touch ${MON_DIR}/done

if [ -z "$(ps -aux | grep ceph-mon | grep -v grep 2>/dev/null)" ] ; then
    ceph-deploy --overwrite-conf mon create-initial 
fi

#Just for test(minimum number of required replicas)
#sudo ceph osd pool set data min_size 1

ceph-deploy gatherkeys ${HOSTNAME}

##########################################################################################
# Setup Ceph: OSD
##########################################################################################

for index in 1 2 3 
do 
    let "index--"
    OSD_DIR="${OSD_BASE_DIR}""${index}"
    ceph-deploy --overwrite-conf osd prepare ${HOSTNAME}:${OSD_DIR}
    ceph-deploy osd activate ${HOSTNAME}:${OSD_DIR}
done

ceph-deploy admin ${HOSTNAME}
chmod +r /etc/ceph/ceph.client.admin.keyring
ceph osd pool set rbd pg_num 256
ceph osd pool set rbd pgp_num 256

##########################################################################################
# Check Ceph status
##########################################################################################
echo "Checking ceph cluster status ......"
ceph health

# Start Metadata server if it will be used as file system
ceph-deploy mds create ${HOSTNAME}

# Start RGW if Ceph Object Gateway will be used
ceph-deploy rgw create ${HOSTNAME}

echo "Ceph has been started in /u01/ceph directory ..."
echo "**********************************************************************************"

