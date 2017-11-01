* [Introduction](#1)
* [Benchmark Test](#2)
* [Ansible role](#3)
* [Others](#4)

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
- [Hadoop](https://github.com/open-estuary/appbenchmark/tree/master/apps/hadoop/hadoop_test1)
- [Flume](https://github.com/open-estuary/appbenchmark/tree/master/apps/flume/flume-ng)
- e-commerce-solutions
  - [e-commerce-kubernetes](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-kubernetes)
  - [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)
- Docker Networks
  - [Calico](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/calico)
  - [flannel](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/flannel)
  - [weave](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/weave)

## <a name="3">Ansible role</a>
Currently the following applications have been supported:

|Application group|Application Name|Tool description|Role description|
|--|--|--|--|
|Other|[spec_cpu2006](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/spec_cpu2006)|SPEC cpu2006 is a tool for CPU testing| This ansible framework supply deploy and test scripts|
|Other|[FIO](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/fio)|FIO is a tool for testing IOPS for stress testing and verification of hardware|This ansible framework supply deploy and test scripts|
- [Sysbench](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/sysbench):Sysbench is a modular, cross platform, multithreaded benchmark tool designed to evaluate and test database loads under a variety of system parameters.This ansible framework supply deploy and test scripts.
- [LmBench](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/lmbench):Lmbench is a tool for memory testing. This ansible framework supply deploy and test scripts.
- [Stream](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/stream):
- [Iperf/Qperf/NetPerf](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/network)
- [MySQL](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/mysql)
- [MySQL_ecommerce](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/mysql-ecommerce)
- [Redis](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/redis)
- [twemproxy](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/twemproxy)
- [estuaryrepo](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/estuaryrepo)
- [nginx](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/nginx)
- [nginx-reverse-proxy](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/nginx-reverse-proxy)
- [wrk](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/wrk)
- [php-fpm](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/php-fpm)
- [solr](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/solr)
- [jmeter](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/jmeter)
- [docker](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/docker)
- [docker_net](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/docker_net)
- [irqbalance](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/irqbalance)
- [lvs-keepalived](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/lvs-keepalived)
- [openjdk](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/openjdk)
- [weighttp](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/weighttp)
- [kafka](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/kafka)
- [flume](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/flume)

- E-Commerce MicroServices
  - [cart](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/cart)
  - [search](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/search)
  - [order](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/order)
- Spring Cloud MicroServices
  - [zuul-apigateway](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/zuul-apigateway)
  - [eureka-service-discovery](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/eureka-service-discovery)

## <a name="4">Others</a>
- As for how to write new test case for applications, please refer to [how_to_write_newcase](https://github.com/open-estuary/appbenchmark/edit/master/how_to_write_newcase.md)

