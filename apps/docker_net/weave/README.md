* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# Network Benchmark Test 
## <a name="1">Introduction</a>
Weave Net is a powerful cloud native networking toolkit. It creates a virtual network that connects Docker containers across multiple hosts and enables their automatic discovery. Set up subsystems and sub-projects that provide DNS, IPAM, a distributed virtual firewall and more. Net is so easy to use, you may even forget it’s there – we like to call this: ‘invisible infrastructure’.

This is a test about weave network, with 3 tools: iperf, netperf and qperf.

## <a name="2">Benchmark Test</a>
### Test Topology
- `vim ansible/hosts` to change your server and client ip address.
- `vim ansible/group_vars/` to change variable related to test.

### Setup
- `./setup.sh` to setup weave in servers.

### Test 
- `./run_test.sh` to perform benchmark test, include iperf, netperf, qperf.

### Test Results
- Get test result from test_client /tmp/dockernet_test directory.
                                           
## <a name="3">Others</a>

