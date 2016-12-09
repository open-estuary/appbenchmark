#!/bin/bash

#ip="192.168.1.187"

ip=${1}

if [ $# -lt 1 ] ; then 
    echo "Usage: ./run_test.sh {ip}"
    exit 0
fi

./scripts/init_client.sh test

orzdba -mysql -i 10 -rt -L ./orzdba_1.log > /dev/null 2>&1 &

