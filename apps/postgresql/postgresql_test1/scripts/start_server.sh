#!/bin/bash

if [ $# -lt 2 ]; then
fi

BASE_DIR=$(cd ~; pwd)
PGHOME=/usr/local/postgresql
PGDATA=/u01/postgresql/data
PGLOGDIR=/u01/postgresql/log
PGGROUP=postgres
PGUSER=postgres

##################################################################################################
# Define system variables 
##################################################################################################
if [ -z "$(grep "PGHOME=" /etc/profile)" ] ; then
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

$(tool_add_sudo) groupadd ${PGGROUP}
$(tool_add_sudo) useradd  ${PGUSER}

${BGHOME}/bin/initdb -D ${PGDATA} -E UTF8 --locale=C -U postgres -X ${PGLOGDIR}


echo "**********************************************************************************"

