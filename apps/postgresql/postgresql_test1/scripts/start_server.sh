#!/bin/bash

BASE_DIR=$(cd ~; pwd)
PGHOME=/usr/local/postgresql
PGDATA=/u01/postgresql/data
PGTRANLOGDIR=/u01/postgresql/log
PGPORT=5432
DEFAULTPASSWD=postgres
TESTUSER=postgres
TESTDB=postgres

##################################################################################################
# Define system variables 
##################################################################################################
if [ -z "$(grep 'PGHOME=' /etc/profile)" ] ; then
    echo "export PGHOME=${PGHOME}" >> /etc/profile
    echo "export PGDATA=${PGDATA}" >> /etc/profile
    echo 'export PATH=$PATH':"$PGHOME/bin" >> /etc/profile
    echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH':"${PGHOME}/lib" >> /etc/profile
fi
source /etc/profile

##################################################################################################
# Initialize database 
##################################################################################################

# Make sure that file system have been formatted and mounted properly
# 
# DISK=/dev/sda
# parted -s ${DISK} mklabel gpt
# parted -s ${DISK} mkpart primary 1MiB 100%
# mkfs.ext4 /dev/sda1 -m 0 -O extent,uninit_bg -E lazy_itable_init=1 -T largefile -L u01
# 
# vim /etc/fstab (mount)
# LABEL=u01 /u01     ext4        defaults,noatime,nodiratime,nodelalloc,barrier=0,data=writeback    0 0
#

if [ ! -d "/u01" ] ; then
    echo "Please mount disk to /u01 directory firstly" 
    exit 0
fi

##########################################################################################
# Initialize Database
##########################################################################################
CURUSER=`whoami`
if [ ! -d ${PGDATA} ] ; then
    sudo mkdir -p ${PGDATA}
fi

if [ ! -d ${PGTRANLOGDIR} ] ; then
    sudo mkdir -p ${PGTRANLOGDIR}
fi

sudo chown -L -R ${CURUSER} ${PGDATA}
sudo chown -L -R ${CURUSER} ${PGTRANLOGDIR}

sudo chmod 700 ${PGDATA}
sudo chmod 700 ${PGTRANLOGDIR}

if [ ! -f ${PGDATA}/postgresql.conf ] ; then
    ${PGHOME}/bin/initdb -D ${PGDATA} -E UTF8 --locale=C -U ${CURUSER} -X ${PGTRANLOGDIR}
else
    echo "PostgreSQL database has been initialized before"
fi

echo "Initialize db with user:${CURUSER}"

CUR_DIR=$(cd `dirname $0`; pwd)
if [ x"${1}" == x"small" ] ; then
    sudo cp ${CUR_DIR}/../config/postgresql_small.conf ${PGDATA}/postgresql.conf
else
    sudo cp ${CUR_DIR}/../config/postgresql.conf ${PGDATA}/postgresql.conf
fi

########################################################################################
# Just set no "password" to test only 
########################################################################################
sudo cp ${CUR_DIR}/../config/pg_hba.conf ${PGDATA}/pg_hba.conf
sudo sed -i 's/host.*all.*all.*md5/host\ all\ all\ 0\.0\.0\.0\/0\ trust/g' ${PGDATA}/pg_hba.conf

echo "Start server right now ..."
if [ -d ${PGDATA}/pg_log ] ; then
    sudo rm -fr ${PGDATA}/pg_log/postgresql*.csv
fi

${PGHOME}/bin/pg_ctl -D ${PGDATA} start 
READY_STR="database system is ready to accept connections"
index=0
while [[ ${index} -lt 10 ]] ; do
    CHECK_READY=$(grep "${READY_STR}" ${PGDATA}/pg_log/postgresql*.csv)
    if [ ! -z "${CHECK_READY}" ] ; then
        break
    fi
    sleep 5
    let "index++"
done

echo "Create Default Database"
${PGHOME}/bin/psql -h 127.0.0.1 -p ${PGPORT} -U ${CURUSER} -d template1 << EOF
CREATE DATABASE ${CURUSER} WITH OWNER ${CURUSER} ENCODING 'UTF8';
CREATE USER ${TESTUSER} WITH PASSWORD 'postgres';
CREATE DATABASE ${TESTUSER} WITH OWNER ${TESTUSER} ENCODING 'UTF8';
EOF

#sudo sed -i 's/host.*all.*all.*trust/host\ all\ all\ 0\.0\.0\.0\/0\ md5/g' ${PGDATA}/pg_hba.conf

#######################################################################################
# Start PostgreSQL Server 
#######################################################################################
#${PGHOME}/bin/pg_ctl -D ${PGDATA} start 
if [ "$(ps -aux | grep postgre | grep -v grep)" ] ; then
    echo "Start PostgreSQL successfully!"
else
    echo "Fail to start PostgreSQL !!!"
fi
echo "**********************************************************************************"

