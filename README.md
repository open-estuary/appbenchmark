* [Introduction](#1)
* [Benchmark Test](#2)
* [Ansible role](#3)
* [Others](#4)

# Estuary Application Benchmark Test 
## <a name="1">Introduction</a>
It is to maintain application benchmark test scripts including:
- Scripts which setup server and client 
- Scripts which perform benchmark test  

>  Bebfore running Ansible, it is necessary to create new account on target servers. Then the later test could use this account to login in target servers without password.
>  As for how to create new accounts automatically, please refer to [createuser.sh](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/createuser.sh)

>  It is required to write new benchmark scripts based on `ansible` framework. In addition, some existing old scripts might be changed to `ansible` framework in the future. As for test cases based on `ansible` framework, it might be necessary to change some configurations before setting up benchmark test based on real test environment, such as: `ansible/hosts` which specifies test hosts,
>   and `ansible/group_vars/*` which define variables for this test.
> 
>  In addition, the `run_test.sh` might only provides some basic test guideline, and more tests are still required to 
>   be performed manually.

>   In addition, it could run test scripts which are based on `ansible` framework on any client server. Usually the client server uses `estuaryapp` or `root` account to login target test servers via ssh without password. Therefore it is required to create `estuaryapp` account firstly on target servers and copy the `id_rsa.pub` from client server to target server in order to login without password.

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
- ARM64 Based E-Commerce Application Solutions
  - [E-Commerce-SpringCloud-Microservice Solution](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)
  - [E-Commerce-Kubernetes Solution](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-kubernetes)
- Docker Networks
  - [Calico](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/calico)
  - [Flannel](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/flannel)
  - [Weave](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/weave)

## <a name="3">Ansible Roles</a>
Currently the following applications have been supported:

|Application Name|Tool description|Role description|
|--|--|--|
|[spec_cpu2006](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/spec_cpu2006)|SPEC cpu2006 is a tool for CPU testing.| This role installs and configures the SPEC CPU2006.|
|[FIO](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/fio)|FIO is a tool for testing IOPS for stress testing and verification of hardware.|This role installs and configures the FIO tool.|
|[Sysbench](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/sysbench)|Sysbench is a modular, cross platform, multithreaded benchmark tool designed to evaluate and test database loads under a variety of system parameters.|This role installs and configures the sysbench tool.|
|[LmBench](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/lmbench)|Lmbench is a tool for memory testing.| This role installs and configures the lmbench tool.|
|[Stream](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/stream)|Stream is a tool for memory testing. Autually, stream is a subprogram of Lmbench.|This role installs and configures the stream tool.|
|[Iperf/Qperf/NetPerf](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/network)|Iperf/Qperf/NetPerf are tools for network.|This role installs and configures the netwoek testing tool.|
|[MySQL](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/mysql)|MySQL is a relational database management system.|This This role installs and configures MySQL to test benchmark performance.|
|[MySQL_ecommerce](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/mysql-ecommerce)|MySQL is a relational database management system.|This role installs and configures MySQL in E-Commerce scenes.|
|[Redis](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/redis)|Redis is a NO_SQL database management system.|This role installs and configures the redis database.|
|[twemproxy](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/twemproxy)|Twemproxy is a fast and lightweight proxy for memcached and redis protocol.|This role installs and configures the twemproxy tool.|
|[estuaryrepo](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/estuaryrepo)|estuaryrepo is a basic role to configre Estuary repository.|This role configures Estuary repository information before testing other functions.|
|[nginx](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/nginx)|Nginx is an HTTP and reverse proxy server, a mail proxy server, and a generic TCP/UDP proxy server.|This role installs and configures the nginx web server.|
|[nginx-reverse-proxy](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/nginx-reverse-proxy)|Nginx is an HTTP and reverse proxy server, a mail proxy server, and a generic TCP/UDP proxy server.|This role installs and configures the nginx web server for reverse proxy.|
|[wrk](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/wrk)|wrk(Windows Research Kernel) is a HTTP benchmarking tool.|This role installs and configures the wrk tool.|
|[php-fpm](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/php-fpm)|PHP-FPM is a Fast-CGI frontend for php, and an enhancement of php-cgi.|This role installs and configures the php-fpm tool.|
|[solr](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/solr)|Solr is the popular, blazing-fast, open source enterprise search platform built on Apache Lucene.|This role installs and configures the solr tool.|
|[jmeter](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/jmeter)|Apache JMeter is a pure Java application designed to test and measure performance.|This role installs and configures the jmeter tool.|
|[docker](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/docker)|docker is the world's leading software containerization platform.|This role installs docker tool.|
|[docker_net](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/docker_net)|docker is the world's leading software containerization platform.|This role installs docker and some network test tools.|
|[irqbalance](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/irqbalance)|Irqbalance is a daemon to help balance the cpu load generated by interrupts across all of a systems cpus.|This role installs and configures irqbalance tool.|
|[openjdk](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/openjdk)|OpenJDK is a open Java Development kit.|This role installs openjdk.|
|[weighttp](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/weighttp)|Weighttp is a lightweight and simple webserver benchmarking tool.|This role installs and configures weighttp tool, and to tests webserver benchmark.|
|[kafka](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/kafka)|Apache Kafka is a distributed streaming platform.|This role installs and configures kafka tool.|
|[flume](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/flume)|Flume is a distributed, reliable, and available service for efficiently collecting, aggregating, and moving large amounts of streaming event data.|This role installs and configures flume tool.|
|[cart](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/cart)|Cart is a MicroServices of E-Commerce.|This role installs and configures cart MicroServices.|
|[search](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/search)|Search is a MicroServices of E-Commerce.|This role installs and configures search MicroServices.|
|[order](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/order)|Order is a MicroServices of E-Commerce.|This role installs and configures order MicroServices.|
|[zuul-apigateway](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/zuul-apigateway)|Zuul is Spring Cloud API Gateway| Install zuul-apigateway MicroServices.|
|[eureka-service-discovery](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/eureka-service-discovery)|Eureka is service discovery microservice of Spring Cloud|This role installs and configures eureka-service-discovery MicroServices.|
||||

## <a name="4">Others</a>
- As for how to write new test case for applications, please refer to [how_to_write_newcase](https://github.com/open-estuary/appbenchmark/edit/master/how_to_write_newcase.md)

