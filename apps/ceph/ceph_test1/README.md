* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# Ceph Benchmark Test 
## <a name="1">Introduction</a>

## <a name="2">Benchmark Test</a>
### Test Topology

### Setup
- Server Side: execute `./run_server.sh` 
- Client Side: execute `./run_test.sh` or `./setup.sh client` 

### Test 
Usually there are four kinds of test cases, that is:
- Basic: 
  - Description: It uses internal `rados bench` tools to test ceph storage pool performance
  - Usage: `./run_test.sh basic`
- Block:
  - Description: It uses `rbd bench` or `fio` to test ceph block device performance
  - Usage: `./run_test.sh block <monitor_server_name/ip>`
- Fio:
  - Description: It uses 
  - Usage:
- Object: 
  - Description:
  - Usage:
    - Server side: execute `run_test_object_server.sh` to setup rgw gateway firstly 
    - Client side: execute `./run_test.sh object <monitor_server_name/ip>` 

### Test Results
                                           
## <a name="3">Others</a>
- Toos Installation 
  - Please use the [estuary.repo](https://github.com/open-estuary/distro-repo/blob/master/utils/estuary.repo) to install the corresponding packages such as fio, ceph and so on.
    Otherwise some tools might fail (such as old fio might not support rbd engine). 

