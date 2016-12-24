#!/bin/bash

src_dir=${1}
dst_dir=${2}

numafile_list=(
"hadoop-yarn-project/hadoop-yarn/hadoop-yarn-server/hadoop-yarn-server-nodemanager/src/main/java/org/apache/hadoop/yarn/server/nodemanager/containermanager/launcher/ContainerLaunch.java"
)

for numafile in ${numafile_list[@]} 
do
echo "Copy file from ${1}/${numafile} to ${2}/${numafile}"
dir_str=${numafile%/*}
mkdir -p ${dst_dir}/${dir_str}
cp ${src_dir}/${numafile} ${dst_dir}/${numafile}

done

