#!/bin/bash

GITHUB_URL=https://github.com/coreos/flannel/releases/download/
DOWNLOAD_URL=${GITHUB_URL}
rm -r /tmp/flannel-{{ FLANNEL_VER }}-linux-{{ Arch }}.tar.gz
rm -rf /tmp/flannel-download-test && mkdir -p /tmp/flannel-download-test

curl -L ${DOWNLOAD_URL}/{{ FLANNEL_VER }}/flannel-{{ FLANNEL_VER }}-linux-{{ Arch }}.tar.gz -o /tmp/flannel-{{ FLANNEL_VER }}-linux-{{ Arch }}.tar.gz
tar zxvf /tmp/flannel-{{ FLANNEL_VER }}-linux-{{ Arch }}.tar.gz -C /tmp/flannel-download-test

cp /tmp/flannel-download-test/flanneld /tmp/flannel-download-test/mk-docker-opts.sh /usr/local/bin
