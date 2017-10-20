#!/bin/bash

if [ {{ netarch }} = "weave" ]; then
	eval $(weave env)
fi
docker run -itd --name {{ netarch }}_iperf_clientcon -v /tmp/:/tmp/ {{ image }} /bin/bash /tmp/iperf_clientcon.sh 
