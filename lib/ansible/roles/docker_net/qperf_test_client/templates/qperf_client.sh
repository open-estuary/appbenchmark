#!/bin/bash

if [ {{ netarch }} = "weave" ];then
	eval $(weave env)
fi
docker run -itd --name {{ netarch }}_qperf_clientcon -v /tmp/:/tmp/ {{image}} /bin/bash /tmp/qperf_clientcon.sh
