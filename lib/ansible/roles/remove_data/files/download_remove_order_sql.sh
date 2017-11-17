#!/bin/bash

SQL_FILE_NAME="remove_data_order.mysql.tar.gz"

if [ ! -f "/opt/data/${SQL_FILE_NAME}" ]; then
    wget -O /opt/data/${SQL_FILE_NAME}  wget ftp://repoftp:repopushez7411@117.78.41.188/releases/5.0/otherfiles/${SQL_FILE_NAME}
fi

pushd /opt/data > /dev/null
tar -zxvf /opt/data/${SQL_FILE_NAME}

popd > /dev/null
