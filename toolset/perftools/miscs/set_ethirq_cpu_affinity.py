#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
    E-mail    :    huangjinhua3@huawei.com
    Data      :    2016-09-07 11:34
    Purpose   :    This tool is to set cpu affinity for ethernet interrupts 

"""

import os
import sys
import re
import json
import collections

def set_ethirq_cpu_affinity():

    if len(sys.argv) < 3:
        print("Usage: set_cpu_affinity_ethirq.py <start cpu index> <end cpu index> {clear}")
        return -1
    
    cpu_start = int(sys.argv[1])
    cpu_end = int(sys.argv[2])

    if cpu_start > cpu_end:
        print("The <start cpu index> should be less than <end cpu index> ")
        return -1

    clear_flag = False    
    if len(sys.argv) >= 4 and sys.argv[3] == "clear":
        clear_flag = True
    
    #Read irq number of ethernet
    irq_file = open("/proc/interrupts")

    eth_irq_dict = collections.defaultdict(list)

    for line in irq_file:
        elems = line.strip().split( )
        irq_num = elems[0]
        device = elems[-1]
        
        #Ignoren non ethernet devices
        if not re.search("eth", device):
            continue

	print("%s-%s"%(irq_num, device))

        try :
            irq_num = int(irq_num[:-1])
        except :
            continue

        ethname = device.split('-')[0]
        ethname += device.split('x')[-1]
       
	print("%s-%s"%(irq_num, ethname))
        if not eth_irq_dict.has_key(ethname):
            eth_irq_dict[ethname] = [irq_num]
        else :
            if irq_num not in eth_irq_dict[ethname]:
                eth_irq_dict[ethname].append(irq_num)

    cur_cpu = cpu_start
    for ethname in eth_irq_dict.keys():
        irq_num_list = eth_irq_dict[ethname]
 
        cpu_hex_num = (0x1 << cur_cpu)
        print("cpu:%x"%cpu_hex_num)

        cpu_hex_num = str(hex(cpu_hex_num))
	if cpu_hex_num.startswith('0x') or cpu_hex_num.startswith('0x'):
	    cpu_hex_num = cpu_hex_num[2:]

        for irq_num in irq_num_list:
            if not clear_flag:
                cmd_str = "echo " + str(cpu_hex_num) + " > " + "/proc/irq/"+str(irq_num)+"/smp_affinity"
            else :
                cmd_str = "echo ffffffff > /proc/irq/"+str(irq_num)+"/smp_affinity"

            print("cmd:%s"%cmd_str)

            cmd_str=str(cmd_str)
            ret=os.system(cmd_str)
        cur_cpu += 1
        if cur_cpu > cpu_end:
            cur_cpu = cpu_start


if __name__ == "__main__":
    set_ethirq_cpu_affinity()
