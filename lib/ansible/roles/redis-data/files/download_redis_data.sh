#!/bin/bash

REDIS_DATA_FILE_NAME="r_ec_sku.redis.tar.gz"

if [ ! -f "/opt/${REDIS_DATA_FILE_NAME}" ]; then
    wget -O /opt/${REDIS_DATA_FILE_NAME}  wget ftp://repoftp:repopushez7411@117.78.41.188/releases/5.0/otherfiles/r_ec_sku.redis.tar.gz
fi

pushd /opt > /dev/null
tar -zxvf /opt/${REDIS_DATA_FILE_NAME} 

popd > /dev/null
