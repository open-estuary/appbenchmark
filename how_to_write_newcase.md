* [Introduction](#1)
* [Benchmark Test Scripts Guidelines](#2)
* [Others](#3)

## <a name="1">Introduction</a>
This document will describe how to write new test cases for each application.

## <a name="2">Benchmark Test Scripts Guidelines</a>
Let's suppose that we want to write test case for `packageA`

### File Strucutes Overview ###
Usually the fillowing files and directories need to be prepared for `packageA`:
```
apps
├── packageA
│   └── packageA_test1
│       ├── config
│       │   └── setup_config.json
│       ├── run_test.sh
│       ├── run_client.sh
│       ├── run_server.sh
│       ├── scripts
│       │   ├── xxx_install.sh
│       │   ├── prerequisites.sh
│       │   ├── start_server.sh
│       │   └── start_client.sh
│       ├── setup.sh
│       └── stop.sh

```
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
