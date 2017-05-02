#!/bin/bash

if [ -z "${1}" ]  ; then
    DEPLOY_USER="ceph-deploy"
else
    DEPLOY_USER="${1}"
fi

if [ -z "${2}" ] ; then
    HOSTNAME="$(hostname)"
else 
    HOSTNAME="${2}"
fi

DEPLOY_PASSWORD="ceph-deploy"

sudo useradd -m -s /bin/bash ${DEPLOY_USER}
sudo passwd ${DEPLOY_USER}

echo "${DEPLOY_USER} ALL = (root) NOPASSWD:ALL" | sudo tee /etc/sudoers.d/${DEPLOY_USER}
sudo chmod 0440 /etc/sudoers.d/${DEPLOY_USER}

chsh -s /bin/bash ${DEPLOY_USER}

sudo su - ${DEPLOY_USER} -c "
ssh-keygen
ssh-copy-id ${DEPLOY_USER}@${HOSTNAME}"

