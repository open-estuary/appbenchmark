* [Introduction](#1)
* [Applibcation Benchmark Test](#2)
* [Ansible roles](#3)
* [Others](#4)

# Estuary Application Benchmark Test 
## <a name="1">Introduction</a>
It is to maintain application benchmark test scripts including:
- Scripts which setup benchmark test server and client 
- Scripts which perform benchmark test  

>  Bebfore running Ansible, it is necessary to create new account on target servers. Then the later test could use this account to login in target servers without password.
>  As for how to create new accounts automatically, please refer to [createuser.sh](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/createuser.sh)

>  It is required to write new benchmark scripts based on `ansible` framework. In addition, some existing old scripts might be changed to `ansible` framework in the future. As for test cases based on `ansible` framework, it might be necessary to change some configurations before setting up benchmark test based on real test environment, such as: `ansible/hosts` which specifies test hosts,
>   and `ansible/group_vars/*` which define variables for this test.
> 

## <a name="2">Applibcation Benchmark Test</a>
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
- ARM64 Based E-Commerce Application Solutions
  - [E-Commerce-SpringCloud-Microservice Solution](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)
  - [E-Commerce-Kubernetes Solution](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-kubernetes)
- Docker Networks
  - [Calico](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/calico)
  - [Flannel](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/flannel)
  - [Weave](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/weave)
- [BigData](https://github.com/open-estuary/appbenchmark/tree/master/apps/bigdata)
## <a name="3">Ansible Roles</a>

> Usually the Ansible role need to be configured and could be reused for difference application benchmark tests (or ansible playbook).

Currently the following applications ansible roles have been supported:

|Application Name|Notes|
|--|--|
|[spec_cpu2006](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/spec_cpu2006)| CPU Test Role|
|[FIO](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/fio)| FIO Test Role|
|[Sysbench](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/sysbench)|Sysbench Role|
|[LmBench](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/lmbench)|LmBench|
|[Stream](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/stream)|Stream Test Role|
|[Iperf/Qperf/NetPerf](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/network)| Network Performance Test Role|
|[MySQL](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/mysql)|MySQL Test Role|
|[Percona Server](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/percona-server)|Percona Server Test Role|
|[Redis](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/redis)|Redis Test Role|
|[twemproxy](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/twemproxy)| Twemproxy Test role|
|[estuaryrepo](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/estuaryrepo)|Provision Estuary Repository|
|[nginx](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/nginx)|Nginx Web Server Role|
|[nginx-reverse-proxy](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/nginx-reverse-proxy)|Nginx Load Balancer Role|
|[wrk](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/wrk)|WRK Test Role|
|[php-fpm](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/php-fpm)|PHP-FPM Role|
|[solr](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/solr)|Single Role Role|
|[solrcloud](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/solrcloud)|Single Cloud Role|
|[jmeter](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/jmeter)|Apache JMeter Role|
|[docker](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/docker)|Install Docker Role|
|[docker_net](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/docker_net)|Docker Network(including Flannel, Weave) Roles|
|[irqbalance](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/irqbalance)|Set Irqbalance Role|
|[openjdk](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/openjdk)|Install OpenJDK Role|
|[weighttp](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/weighttp)|Weighthttp Role|
|[kafka](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/kafka)|Apache Kafka Role|
|[flume](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/flume)|Flume Role|
|[cart](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/cart)|E-commerce Cart MicroService Role|
|[search](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/search)|E-commerce Search MicroService Role|
|[order](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/order)|E-commerce Order MicroService Role|
|[zuul-apigateway](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/zuul-apigateway)|Sring Cloud Zuul Role|
|[eureka-service-discovery](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/eureka-service-discovery)|Spring Cloud Eureka Role|
|[createuser](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/createuser)|Create new user account role|
|[mount](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/mount)|Mount new devices role|
|[syscfg](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/syscfg)|Provision Kernel Parameters role|
|[bigdata](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/bigdata)|Bigdata Install Test role|
||||

## <a name="4">Others</a>
- As for how to write new test case for applications, please refer to [how_to_write_newcase](https://github.com/open-estuary/appbenchmark/edit/master/how_to_write_newcase.md)

