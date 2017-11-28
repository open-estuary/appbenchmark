# !/bin/bash

CURDIR=$(cd `dirname $0`; pwd)
pushd ${CURDIR}/ansible > /dev/nul

echo "exec prepose script"
#bash $CURDIR/ansible/scripts/scala/scala_cfg.sh
bash $CURDIR/ansible/scripts/hadoop/hadoop_local_download_cfg.sh
bash $CURDIR/ansible/scripts/spark/spark_local_download_cfg.sh

ansible-playbook -i hosts site.yml 

popd > /dev/null
