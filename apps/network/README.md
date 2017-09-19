* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# Network Benchmark Test 
## <a name="1">Introduction</a>
This is a test about network, include 3 tools: iperf, netperf  and qperf.

- Iperf

  - Primarily used to measure TCP and UDP throughput

- NetPerf

  - Primarily used to measure latency

- Qperf

  - Measure TCP and UDP bandwidth and latencies

## <a name="2">Benchmark Test</a>
### Test Topology
- `vim ansible/hosts` to change your client ip address under NetworkClient group
- `vim ansible/roles/{xxx}/vars/` to change variable 'server_add' to your server ip address

### Setup
- `./setup.sh` to setup tools in client side

### Test 
- `./run_server.sh` to perform benchmark test on server side
- `./run_test.sh` to perform benchmark test on client side

### Test Results
- Get test result from result directory.
                                           
## <a name="3">Others</a>

