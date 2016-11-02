#!/usr/bin/python 
#-*-:encodig=utf-8-*-
"""
   To execute shell commands in parallel
   In addition, each argument stands for one shell command.
   Especially if the argument is one file, each file line (except comment) stands for one shell command
"""

import os
import subprocess
import sys

def parallel_execute_cmd(cmd_str_list):
    max_proc_num = 64
   
    proc_list = []
    for index in range(len(cmd_str_list)):
        if len(proc_list) < max_proc_num:
            print("Begin to execute:%s\n"%cmd_str_list[index])
            proc_list.append(subprocess.Popen(cmd_str_list[index], shell=True))
        else :
            for proc_index in range(len(proc_list)):
                proc_list[proc_index].wait()
            proc_list = []

    for proc_index in range(len(proc_list)):
         proc_list[proc_index].wait()


if __name__ == "__main__":
    cmd_str_list = []
    for index in range(len(sys.argv) - 1):
        if os.path.isfile(sys.argv[index +1]):
            cmd_file = open(sys.argv[index + 1];
            for line in cmd_file:
                line = line.strip()
                if line[0:1] == "#":
                    continue
                cmd_str_list.append(line)
        else :
            cmd_str_list.append(sys.argv[index + 1])

    parallel_execute_cmd(cmd_str_list)

    
