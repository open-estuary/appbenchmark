#!/bin/bash

./run_client.sh wordcount large
./run_client.sh wordcount huge
#./run_client.sh wordcount bigdata


./run_client.sh terasort large
./run_client.sh terasort huge
#./run_client.sh terasort bigdata

./run_client.sh pagerank large
./run_client.sh pagerank huge
#./run_client.sh pagerank bigdata


./run_client.sh dfsioe large
./run_client.sh dfsioe huge
#./run_client.sh dfsioe bigdata

