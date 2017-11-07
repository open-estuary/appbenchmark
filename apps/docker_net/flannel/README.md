* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# Network Benchmark Test 
## <a name="1">Introduction</a>
flannel is a virtual network that gives a subnet to each host for use with container runtimes.
Platforms like Google's Kubernetes assume that each container (pod) has a unique, routable IP inside the cluster. 
The advantage of this model is that it reduces the complexity of doing port mapping.

This is a test about flannel network, with 3 tools: iperf, netperf and qperf.

## <a name="2">Benchmark Test</a>
### Test Topology
- `vim ansible/hosts` to change your server and client ip address.
- `vim ansible/group_vars/` to change variable related to test.

### Setup
- `./setup.sh` to setup flannel in servers.

### Test 
- `./run_test.sh` to perform benchmark test, include iperf, netperf, qperf.

### Test Results
- Get test result from test_client /tmp/dockernet_test directory.
                                           
## <a name="3">Others</a>

