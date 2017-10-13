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
- `./setup.sh` to setup client and server side;
- `./run_{test/xxx}.sh` to perform benchmark test on client side

Currently the following applications have been supported so far :
- Basic Test
  - CPU
    - [SpecInt20016](https://github.com/open-estuary/appbenchmark/tree/master/apps/cpu/spec_cpu2006)
  - IO
    - [FIO](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/fio)
    - [Sysbench](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/sysbench)
  - Memory
    - [LmBench](https://github.com/open-estuary/appbenchmark/tree/master/apps/memory/lmbench_1)
    - [Stream](https://github.com/open-estuary/appbenchmark/tree/master/apps/memory/stream_1)
  - Network
    - [Iperf/Qperf/NetPerf](https://github.com/open-estuary/appbenchmark/tree/master/apps/network)
  - JVM
    - [SpecJVM2008](https://github.com/open-estuary/appbenchmark/tree/master/apps/jvm/specjvm2008)
  
- MySQL(Percona Server)
  - [Test Case 1(1 large instance test which focus on latency)](https://github.com/open-estuary/appbenchmark/tree/master/apps/mysql/percona_1)
- AliSQL
  - [Test Case 1(200 small instances test)](https://github.com/open-estuary/appbenchmark/tree/master/apps/mysql/alisql_1)
  - [Test Case 2(5 middle instances test)](https://github.com/open-estuary/appbenchmark/tree/master/apps/mysql/alisql_2)
- [Cassandra](https://github.com/open-estuary/appbenchmark/tree/master/apps/cassandra/cassandra_test1)
- [Ceph](https://github.com/open-estuary/appbenchmark/tree/master/apps/ceph/ceph_test1)
- [MariaDB](https://github.com/open-estuary/appbenchmark/tree/master/apps/mariadb/mariadb_test1)
- [PostgreSQL](https://github.com/open-estuary/appbenchmark/tree/master/apps/postgresql/postgresql_test1)
- [MongoDB](https://github.com/open-estuary/appbenchmark/tree/master/apps/mongodb/mongodb_test1)
- [Redis](https://github.com/open-estuary/appbenchmark/tree/master/apps/mongodb/mongodb_1)   
- [Solr](https://github.com/open-estuary/appbenchmark/tree/master/apps/solr)
- Docker Networks
  - [Calico](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/calico)
  - [flannel](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/flannel)
  - [weave](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/weave)

## <a name="3">Others</a>
- As for how to write new test case for applications, please refer to [how_to_write_newcase](https://github.com/open-estuary/appbenchmark/edit/master/how_to_write_newcase.md)

