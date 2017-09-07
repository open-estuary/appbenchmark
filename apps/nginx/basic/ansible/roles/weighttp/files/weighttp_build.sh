#!/bin/bash

src_dir=$1

pushd ${src_dir} > /dev/null
./waf configure --prefix=/usr
./waf build 16
./waf install 
popd > /dev/null

