#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
    E-mail    :    huangjinhua3@huawei.com
    Data      :    2016-09-07 11:34
    Purpose   :    This tool is to perform install commands according to the specified json file

"""

import os
import sys
import stat
import shutil
import re
import json
import subprocess

########################################################################
# As for local script, it will add "./" prefix.
# As for system commands, it will use them directly
########################################################################
def get_execute_cmd(inputcmd):
    if os.path.exists("./"+inputcmd) :
        return "./" + inputcmd
    else :
        return inputcmd

def main():
    #Add current directory to shell path 
    os.environ['PATH'] = os.environ['PATH'] + os.getcwd()

    if len(sys.argv) < 3:
        print("Usage: setup.py <cfg.json file> {client|server}")
        return -1

    try :
        APP_ROOT_DIR = os.path.join(os.environ['APP_ROOT'])
    except :
        cur_dir = os.path.dirname(sys.argv[0])
        APP_ROOT_DIR = os.path.join(os.path.join(cur_dir, os.path.pardir), os.path.pardir)

    json_str = ""
    json_data = ""
    try :
        json_file = open(sys.argv[1])
        for line in json_file:
            json_str += line.strip()

        json_data = json.loads(json_str)

    except Exception as ex:
        print("Fail to process % due to error:%s"%(sys.argv[1], ex))
        return -1

    install_elem = ""
    if re.search("server", sys.argv[2]):
        install_elem = "server_setup"
    elif re.search("client", sys.argv[2]):
        install_elem = "client_setup"
    else :
        print("%s is invalid parameter. It should be server or client so far")
        return -1


    if len(json_data[install_elem]) == 0:
        print("%s json file has no %s provisioning"%(sys.argv[1], sys.argv[2]))
        print("**************************************************************")
        return -1

    try :
        print("");
        print("************************************************************************")
        
        for index in range(len(json_data[install_elem])):
            package_name = json_data[install_elem][index]["name"]
            if package_name == "":
                continue

            download_url = json_data[install_elem][index]["download_url"]
            local_filename = json_data[install_elem][index]["local_filename"]

            target_filename=local_filename
            if download_url != "" :
                print("************************************************************************")
                print("%s::step1:begin to download %s ......"%(package_name, download_url))
                if target_filename == "":
                    target_filename=download_url.strip().split('/')[-1]

                if os.path.exists("./"+target_filename):
                    print("%s already exist, so it would not be downloaded again"%target_filename)
                elif local_filename == "":
                    subprocess.check_call(["curl", "-sL", "-O", download_url])
                else:
                    subprocess.check_call(["curl", "-sL", "-o", target_filename, download_url])

            pre_install_cmd = json_data[install_elem][index]["pre_install_cmd"]
            if pre_install_cmd != "":
                print("************************************************************************")
                print("%s::step2:begin to execute %s"%(package_name, pre_install_cmd))
                subprocess.check_call([get_execute_cmd(pre_install_cmd), target_filename])
            else :
                print("************************************************************************")
                print("%s::step2:passed"%package_name)

            build_cmd = json_data[install_elem][index]["build_cmd"]
            if build_cmd != "":
                print("************************************************************************")
                print("%s::step3:begin to execute %s"%(package_name, build_cmd))
                subprocess.check_call([get_execute_cmd(build_cmd), target_filename])
            else :
                print("************************************************************************")
                print("%s::step3:passed"%package_name)

            install_cmd = json_data[install_elem][index]["install_cmd"]
            if install_cmd != "":
                print("************************************************************************")
                print("%s::step4:begin to execute %s"%(package_name, install_cmd))
                subprocess.check_call([get_execute_cmd(install_cmd), target_filename])
            else :
                print("************************************************************************")
                print("%s::step4:passed"%package_name)

    except Exception as ex:
        print("Fail to install package due to %s"%ex)

    print("************************************************************************")
    print("***************** Setup scripts have been completed ********************")
    print("************************************************************************")

if __name__ == "__main__":
    main()
