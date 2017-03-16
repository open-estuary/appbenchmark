#!/bin/bash

export LD_LIBRARY_PATH=/usr/local/lib64:/usr/local/lib:${LD_LIBRARY_PATH}

FILESIZE=1000
THREADS=16

echo "{nThreads:${THREADS},fileSizeMB:${FILESIZE},r:true,w:true}" | /usr/local/mongodb/bin/mongoperf 
