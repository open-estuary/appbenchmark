#!/bin/bash

PGDATA="/u01/postgresql/data/"
sudo kill -INT `head -1 ${PGDATA}/postmaster.pid`

