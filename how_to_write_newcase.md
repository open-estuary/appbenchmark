* [Introduction](#1)
* [](#2)
* [Others](#3)

## <a name="1">Introduction</a>
This document will describe how to write new test cases for each application.

## <a name="2">Build</a>
Let''s suppose that we want to write test case for `packageA`

### File Strucutes Overview ###
Usually the fillowing files and directories need to be prepared:
   *[apps](./apps)
    *[packageA](./apps/packageA)
      *[packageA_test1)(./apps/packageA/packageA_test1)
       *[setup.sh](./apps/packageA/packageA_test1/setup.sh)
       *[run_server.sh](./apps/packageA/packageA_test1/run_server.sh)
       *[run_client.sh](./apps/packageA/packageA_test1/run_client.sh)
       *[run_test.sh](./apps/packageA/packageA_test1/run_test.sh)
       *[config](./apps/packageA/packageA_test1/config)
         *[setup_config.json](./apps/packageA/packageA_test1/config/setup_config.json)
       *[scripts](./apps/packageA/packageA_test1/scripts)
         *[build/setup_package.sh](./apps/packageA/packageA_test1/scripts/build_setuppackage.sh)

### Test Setup Scripts
The `setup.sh` will calls the corresponding `setup_config.json` to setup server and client sides.
There are some guidelines to help write `setup_config.json` properly as follows:
  - usually each elment will build and install one package such as this package itself or other required packages
  - the `download_url` indicate file url address which will be downloaded automatically. Please note that it doesnot support github url address so far
  - the `pre_install_cmd` specify scripts which are executed before building procedure
  - the `build_cmd` and `install_cmd` specify scripts which are to build and install packages accordingly
  - usually these scripts are located under above scripts directory such as `apps/packageA/packageA_test1/scripts`. On other other hand, it is also strongly suggested to reuse commont scripts which is located under `apps/common/scripts` directory. In order to reuse common scripts, the values of `xxx_cmd` MUST include `common/`. For example:
    - `build_cmd: build_packagea.sh`: the `build_package.sh` is located under `apps/packageA/packageA_test1/scripts`
    - `build_cmd: common/build_common.sh` : the `build_common.sh` is located under `apps/common/scripts`
  - if any value is not required, it could be specified as ""

In addition, the `run_servers.sh` and `run_client.sh` will be called to setup server and client test sides accordingly. 

### Test Exection Scripts
Usually the `run_test.sh` will be executed to run benchmark test case on client side. 

## <a name="3">Others</a>
