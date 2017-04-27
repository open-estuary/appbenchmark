#!/bin/bash

yum install ntp ntpupdate -y
ntpdate pool.ntp.org
systemctl restart netdate.service
systemctl restart ntpd.service
systemctl enable ntpd.service
systemctl enable ntpdate.service

