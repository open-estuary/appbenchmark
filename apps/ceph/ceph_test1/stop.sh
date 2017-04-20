#!/bin/bash

PGDATA="/u01/ceph/data/"
sudo kill -INT `head -1 ${PGDATA}/postmaster.pid`

