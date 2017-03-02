#!/usr/bin/python

import os
import sys
import collections
import re

def analysis_sysbench_test(dirname):
    rst_dict = collections.defaultdict(dict)    
 
    testcases_dirs = os.listdir(dirname)

    for casename in testcases_dirs :
        filepath = os.path.join(dirname, casename)
        if not os.path.isdir(filepath):
            continue
        
        threadnum_dirs = os.listdir(filepath)
        for thread_num in threadnum_dirs:
            testfilename = os.path.join(filepath, thread_num)
            if not os.path.isdir(testfilename):
                continue
            
            final_files = os.listdir(testfilename)
            for final_file in final_files:
                final_filename = os.path.join(testfilename, final_file)
                if os.path.isdir(final_filename):
                    continue
                tps = 0
                latency = 0.0
                filehandle = open(final_filename)
                for line in filehandle:
                    elems = re.split(" +", line.strip())
                    #print("%s"%elems)
                    if len(elems) < 2:
                        continue
                    if re.search("transactions", elems[0]):
		        tps = float(elems[2][1:])
                    elif re.search("95th", elems[0]):
                        latency = float(elems[2])

                if int(tps) == 0 and int(latency) == 0:
                    continue 
                
                if not rst_dict[casename].has_key(thread_num):
                    rst_dict[casename][thread_num] = collections.defaultdict(dict)

                rst_dict[casename][thread_num]["tps"] = tps + rst_dict[casename][thread_num].get("tps", 0)
                rst_dict[casename][thread_num]["latency"] = latency + rst_dict[casename][thread_num].get("latency", 0)
                rst_dict[casename][thread_num]["num"] = 1 + rst_dict[casename][thread_num].get("num", 0)

    print("%-25s%-10s%-10s%-10s%-10s"%("Case", "ThreadNum", "TPS", "Latency", "TestNum"))
    for casename in rst_dict.keys():
        for thread_num in rst_dict[casename].keys():
            tps = rst_dict[casename][thread_num]["tps"]
            latency = rst_dict[casename][thread_num]["latency"]
            num = rst_dict[casename][thread_num]["num"]
    
            print("%-25s%-10s%-10.2f%-10.2f%-10d"%(casename, thread_num, float(tps)/float(num), float(latency)/float(num), num))
    
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: analysis_sysbench.sh <test_result_dir>")
    else : 
        analysis_sysbench_test(sys.argv[1])


