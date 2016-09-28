#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
    E-mail    :    huangjinhua3@huawei.com
    Data      :    2016-09-07 11:34
    Purpose   :    This tool is to generate dump data for redis test

"""

import os
import sys
import re

def generate_dump_data():

    if len(sys.argv) < 2:
        print("Usage: generate_inputdata.py <outputfilename> {data_len}")
        return -1

    outputfile = open(sys.argv[1], 'w')
    data_len = 10000

    if len(sys.argv) >= 3:
        data_len = int(sys.argv[2])

    for index in range(data_len):
        outputfile.write("SET key:%012d XXXXXXXXXXXXX%d\r\n"%(index, index))
  
    print("Finish generate dump data");

if __name__ == "__main__":
    generate_dump_data()
