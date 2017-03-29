#!/bin/bash

/usr/local/cassandra/bin/cqlsh << EOF
DROP TABLE if exists ycsb.usertable;
DROP KEYSPACE if exists ycsb;
EOF
