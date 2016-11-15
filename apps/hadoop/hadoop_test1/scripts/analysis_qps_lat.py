#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
    E-mail    :    huangjinhua3@huawei.com
    Data      :    2016-10-08 11:34
    Purpose   :    This tool is to analysis hadoop-benchmark results

"""

import os
import sys
import re


def get_hadoopbenchmark_logs(cur_dir):
    """
    To get all hadoop-benchmark logs under cur_dir
    """
    file_lists = os.listdir(cur_dir)
    logfile_list = []
    for filename in file_lists:
        if re.match("hadoop", filename):
            filepath = os.path.join(cur_dir, filename)
            logfile_list.append(filepath)
    return logfile_list


def analysis_hadoopbenchmark_qps_lat():

    if len(sys.argv) < 2:
        print("Usage: analysis_qps_lat.py <logfile_dir>")
        return -1

    logfile_list = get_hadoopbenchmark_logs(sys.argv[1])

    qps_list =[]
    lat_list =[]
    for filename in logfile_list:
        logfile = open(filename)

        cur_qps = 0.0
        cur_lat = 0.0
        
        has_got_value = False
        for line in logfile:
            elems = line.strip().split( )
            if len(elems) < 4:
                continue

            if elems[1] == "requests" and elems[2] == "per" and elems[3] == "second":
                cur_qps = float(elems[0])
            
            #print("%s,%s"%(elems[0][:-1], elems[2]))
            #Currently we will only care about latency at 90% level 
            try :
                perf_value = float(elems[0][:-1])
                lat_value = float(elems[2])
            except :
                continue

            if perf_value >= 90.0 and elems[3] == "milliseconds":
                if not has_got_value:
                    has_got_value = True
                    cur_lat = lat_value

        if has_got_value:
            qps_list.append(cur_qps)
            lat_list.append(cur_lat)

            print("qps:%0.2f, lat:%0.2f"%(cur_qps, cur_lat))

    if len(qps_list) == 0:
        print("Operation is still in progress ......")
        return

    total_qps = 0.0
    total_lat = 0.0
    for index in range(len(qps_list)):
        total_qps += float(qps_list[index])
        total_lat += float(lat_list[index])

    print("Total qps:%0.2f, Avg lat:%0.2f"%(total_qps, total_lat/len(lat_list)))

if __name__ == "__main__":
    analysis_hadoopbenchmark_qps_lat()
