* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# Estuary Application Benchmark Test 
## <a name="1">Introduction</a>
It is to maintain application benchmark test scripts including:
- Scripts which setup server and client 
- Scripts which perform benchmark test  

## <a name="2">Benchmark Test</a>
Usually it is only necessary to execute following commands in order to perform benchmark test for each application:
- `./run_server.sh` to setup server on server side
- `./run_client.sh` to setup client on client side
- `./run_{test/xxx}.sh` to perform benchmark test on client side

Currently the following applications have been supported so far :
- MySQL(Percona Server)
  - Test Case 1(1 large instance test which focus on latency) [ ]
- AliSQL
  - Test Case 1(200 small instances test) [ ]
  - Test Case 2(5 middle instances test) []
- MariaDB
- PostgreSQL
- Cassandra
- MongoDB
- Ceph
                                           
## <a name="3">Others</a>
- As for how to write new test case for applications, please refer to []
- Any Questions, please 
