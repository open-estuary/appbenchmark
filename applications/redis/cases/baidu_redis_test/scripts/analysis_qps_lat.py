#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
    E-mail    :    huangjinhua3@huawei.com
    Data      :    2016-10-08 11:34
    Purpose   :    This tool is to analysis redis-benchmark results

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

    for index in range(data_num):
        outputfile.write("SET key:%012d %s\r\n"%(index, key_value))
  
    print("Finish generate dump data");

if __name__ == "__main__":
    generate_dump_data()
