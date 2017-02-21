#!/bin/bash

CUR_DIR=$(cd `dirname $0`; pwd)
PASSWORD=$1
INDEX=0
CMD_STR=""

for var in $@
do
    if [ ${INDEX} -gt 0 ] ; then
        CMD_STR="${CMD_STR}"" ""$var"
    fi
    
    let "INDEX++"
done

TMPFILE="/tmp/expectcmd/tmp_cmd.sh"
if [ ! -d "/tmp/expectcmd" ] ; then
    mkdir "/tmp/expectcmd"
fi

echo "#!bin/bash" > ${TMPFILE}
echo "${CMD_STR}" >> ${TMPFILE}
chmod 755 ${TMPFILE}

${CUR_DIR}/expect_cmd.exp ${PASSWORD} sh ${TMPFILE}
   
