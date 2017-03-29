#!/bin/bash

PGDATA="/u01/cassandra/data/"
sudo kill -INT `head -1 ${PGDATA}/postmaster.pid`

