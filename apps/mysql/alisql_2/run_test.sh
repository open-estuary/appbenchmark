#!/bin/bash

if [ -z "${1}" ] ; then
    echo "Usage: ./run_client.sh {ip}"
fi

./scripts/init_client.sh ${1} test 200   > ./testresult_200_1thread
./scripts/init_client.sh ${1} test 200 4 > ./testresult_200_4thread
./scripts/init_client.sh ${1} test 200 8 > ./testresult_200_8thread

