#!/bin/bash


if [ {{ netarch }} = "weave" ]; then
	eval $(weave env)
fi
docker run -itd --name {{ netarch }}_iperf_servercon -v /tmp/:/tmp/ {{ image }} /bin/bash /tmp/iperf_servercon.sh
