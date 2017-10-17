#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
    E-mail    :    huangjinhua3@huawei.com
    Data      :    2016-10-08 11:34
    Purpose   :    This tool is to generate dump data for redis test

"""

import os
import sys
import re

def generate_dump_data():

    if len(sys.argv) < 2:
        print("Usage: generate_inputdata.py <outputfilename> {data_num} {data_size}")
        return -1

    outputfile = open(sys.argv[1], 'w')
    data_num = 10000

    if len(sys.argv) >= 3:
        data_num = int(sys.argv[2])

    data_size = 10
    if len(sys.argv) >= 4:
        data_size = int(sys.argv[3])

        key_value="X"*data_size
        #key_value="{\'nshoppingcartid\':6,\'userId\':1,\'discount\':990.00,\'price\':13392.00,\'quantity\':18,\'currency\':\'RMB\',\'skudtoList\':[{\'skuId\':10001,\'spuId\':1001,\'color\':\'白色\',\'size\':\'4GB+128GB\',\'price\':799.00,\'displayPrice\':799.00,\'currency\':\'RMB\',\'discount\':99.00,\'dcreatetime\':1506385918000,\'dupdatetime\':1506385918000,\'quantity\':10},{\'skuId\':10002,\'spuId\':1001,\'color\':\'金色\',\'size\':\'6GB+128GB\',\'price\':799.00,\'displayPrice\':799.00,\'currency\':\'RMB\',\'discount\':0.00,\'dcreatetime\':1506385930000,\'dupdatetime\':1506385930000,\'quantity\':8}],\'supdatetime\':1506393229000,\'ncreatetime\':1506393229000}"

    for index in range(data_num):
        outputfile.write("SET key:%012d %s\r\n"%(index, key_value))
  
    print("Finish generate dump data");

if __name__ == "__main__":
    generate_dump_data()
