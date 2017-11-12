#!/bin/bash

yum clean all
yum install iperf -y

iperf -s -w 256k 
