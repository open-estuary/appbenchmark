#!/bin/bash

work_dir="${1}"
version="${2}"

src_file=apache-jmeter-${version}.tgz
install_dir="/opt/jmeter"

if [ ! -f "${work_dir}/${src_file}" ] ; then
    wget -O "${work_dir}/${src_file}" http://mirrors.tuna.tsinghua.edu.cn/apache//jmeter/binaries/${src_file}
fi

if [ ! -d "/opt" ] ; then
    mkdir -p "/opt"
fi

if [  -d "${install_dir}" ] ; then
    rm -r "${install_dir}"
fi

tar -zxvf ${work_dir}/${src_file} -C "${work_dir}"
mv "${work_dir}/apache-jmeter-${version}" "${install_dir}"


