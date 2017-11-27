#!/bin/bash

SQL_TAR_FILE_NAME="r_ec_cart.mysql.tar.gz"
SQL_FILE_NAME="r_ec_cart.mysql"

if [ ! -f "/opt/data/${SQL_TAR_FILE_NAME}" ]; then
    wget -O /opt/data/${SQL_TAR_FILE_NAME}  wget ftp://repoftp:repopushez7411@117.78.41.188/releases/5.0/otherfiles/${SQL_TAR_FILE_NAME}
fi

pushd /opt/data > /dev/null
tar -zxvf /opt/data/${SQL_TAR_FILE_NAME}
mv ${SQL_FILE_NAME}/* ./
rm -rf ${SQL_FILE_NAME}

popd > /dev/null
