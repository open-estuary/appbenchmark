#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys


def save_data_2_redis(filename, contents):
    fd = open(filename, 'a')
    fd.write(contents)
    fd.close()

def convert_data():
    if len(sys.argv) < 1:
        print("Usage: data_convert_redis.py <filename>")
        return -1

    append_character = "|"
    filename = sys.argv[1]

   # num = 0
    fd = open(filename, 'a+')
    for line in fd:
        #ignore the first line
       # if (num != 1):
        #    num = 1
         #   continue
        line = line[:-1]   #去掉最后一个换行符
        fields = line.split('|')
        if (filename == "r_ec_sku.csv"):

            redis_key = "SKU:" + fields[0]   #only 1 primary key

            redis_value = "'"
            i = 1
            for i in range(1, len(fields) - 2) :
                redis_value = redis_value + fields[i]
                redis_value = redis_value + "|"

            last_field = fields[-2]
            last_field.strip()
            redis_value = redis_value + last_field
            redis_value = redis_value + "'\n"
            redis_text = "SET " + redis_key + " " + redis_value
            save_data_2_redis("r_ec_sku.redis.txt", redis_text)

print("begin convert data")
convert_data()
print("end")
