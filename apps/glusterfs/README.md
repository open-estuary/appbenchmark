* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# Network Benchmark Test 
## <a name="1">Introduction</a>
Gluster is a software defined distributed storage that can scale to several petabytes. It provides interfaces for object, block and file storage.

## <a name="2">Benchmark Test</a>
### Test Topology
- `vim ansible/hosts` to change your server and client ip address.
- `vim ansible/group_vars/` to change variable related to test.

### Setup
- `./setup.sh` to setup kubernetes in servers.

## <a name="3">Others</a>

