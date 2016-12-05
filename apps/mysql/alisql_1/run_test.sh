#!/bin/bash

./run_client.sh test 200   > ./testresult_200_1thread
./run_client.sh test 200 4 > ./testresult_200_4thread
./run_client.sh test 200 8 > ./testresult_200_8thread

