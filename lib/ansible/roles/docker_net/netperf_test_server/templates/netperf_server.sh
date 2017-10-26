#!/bin/bash

if [ {{ netarch }} = "weave" ]; then 
	eval $(weave env)
fi
docker run -itd --name {{ netarch }}_netperf_servercon -v /tmp/:/tmp/ {{ image }} /bin/bash /tmp/netperf_servercon.sh
