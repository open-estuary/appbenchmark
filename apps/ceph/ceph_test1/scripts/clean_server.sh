#!/bin/bash

servername=$1
if [ -z "${servername} " ] ; then
    servername=$(hostname)
fi

ceph-deploy purgedata ${servername}
ceph-deploy forgetkeys

ceph-deploy purge ${servername}
