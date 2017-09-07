#!/bin/bash

if [ -z "$(which fio 2>/dev/null)" ] ; then
    echo "Please install fio firstly !"
    exit 0
fi
IP=`ifconfig eth0|sed -n 2p|awk  '{ print $2 }'|tr -d 'addr:'`;
CUR_DIR=$(cd `dirname $0`; pwd)
if [ ! -d "${CUR_DIR}/result" ]; then
  mkdir -p  ${CUR_DIR}/result
fi
IOTEST_DIR="${CUR_DIR}/result"
pushd ${IOTEST_DIR} > /dev/null
rwstyle="randread"
blksectcor="4k"
iodepth=512
echo "
[global]
rw=${rwstyle}
direct=1
ramp_time=1
ioengine=libaio
iodepth=${iodepth}
numjobs=1
bs=${blksectcor}
#;size=102400m
;zero_buffers=1
group_reporting=1
;ioscheduler=noop
;gtod_reduce=1
;iodepth_baitch=2
;iodepth_batch_complete=2
runtime=300
;thread
loops=10
" >${rwstyle}_${blksector}_depth${iodepth}_fiotest
fiofile=${rwstyle}_${blksector}_depth${iodepth}_fiotest

declare -i new_count=1

if [ "$devtype"x = "nvme"x   ];then
    echo /dev/nvme0n1 > devinfo
else
    fdisk -l | grep -o "/dev/[a-z]d[a-z][1-9]" > devinfo
fi

new_num=`sed -n '$=' devinfo`

while [ $new_count -le $new_num ]
do
        new_disk=`sed -n "$new_count"p devinfo`
        ((new_count++))
        if [ "$new_disk" = "/dev/sd" ]; then
continue
        fi
        echo "[job1]" >> $fiofile
        echo "filename=$new_disk" >> $fiofile

done


nohup fio $fiofile >>result_${rwstyle}_${IP} 2>&1 & 


popd > /dev/null

