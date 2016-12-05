#!/bin/bash

if [ ! -f "/u01/my3306/bin/mysqld_safe" ] ; then
    ./setup.sh server
else
    echo "MySql server has been installed before"
fi

./scripts/init_server.sh init 6
./scripts/init_server.sh start 6
