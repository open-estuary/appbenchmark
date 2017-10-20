#!/bin/bash

yum clean all
yum install netperf -y

netserver

while true
do
    sleep 1s
done
