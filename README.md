* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# Estuary Application Benchmark Test 
## <a name="1">Introduction</a>
It is to maintain application benchmark test scripts including:
- Scripts which setup server and client 
- Scripts which perform benchmark test  

> Notes: 
>    It is required to write new benchmark scripts based on `ansible` framework. In addition, some existing old scripts might be changed to `ansible` framework in the future. As for test cases based on `ansible` framework, it might be necessary to change some configurations before setting up benchmark test based on real test environment, such as: `ansible/hosts` which specifies test hosts,
>    and `ansible/group_vars/*` which define variables for this test.
> 
>    In addition, the `run_test.sh` might only provides some basic test guideline, and more tests are still required to 
>    be performed manually.

>    In addition, it could run test scripts which are based on `ansible` framework on any client server. Usually the client server uses `estuaryapp` or `root` account to login target test servers via ssh without password. Therefore it is required to create `estuaryapp` account firstly on target servers and copy the `id_rsa.pub` from client server to target server in order to login without password.

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

