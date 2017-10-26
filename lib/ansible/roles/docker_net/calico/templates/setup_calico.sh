#!/bin/bash

GITHUB_URL=https://github.com/projectcalico/calicoctl/releases/download
DOWNLOAD_URL=${GITHUB_URL}

wget -O /usr/local/bin/calicoctl ${DOWNLOAD_URL}/{{ CALICO_VER }}/calicoctl 
chmod +x /usr/local/bin/calicoctl

calicoctl node run
