* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# Ceph Benchmark Test 
## <a name="1">Introduction</a>
It is to perform [Ceph](https://github.com/ceph/ceph) benchmark tests.

## <a name="2">Benchmark Test</a>
### Setup
Currently it support perform benchmark tests on single node or multiple nodes.
- Server Side: execute `./run_server.sh` 
- Client Side: execute `./run_test.sh` or `./setup.sh client` 

### Test 
Usually there are two kinds of test cases, that is:
- Basic: 
  - Description: It uses internal `rados bench` tools to test ceph storage pool performance
  - Usage: `./run_test.sh basic`
- Block:
  - Description: It uses `rbd bench` or `fio` to test ceph block device performance
  - Usage: `./run_test.sh block <monitor_server_name/ip>`

### Test Results
                                           
## <a name="3">Others</a>
- Toos Installation 
  - Please use the [estuary.repo](https://github.com/open-estuary/distro-repo/blob/master/utils/estuary.repo) to install the corresponding packages such as fio, ceph and so on.
    Otherwise some tools might fail (such as old fio might not support rbd engine). 

