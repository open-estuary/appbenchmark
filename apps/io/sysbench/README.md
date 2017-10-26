* [Introduction](#1)
* [Benchmark Test](#2)

# Sysbench Benchmark Test 
## <a name="1">Introduction</a>
Sysbench is a modular, cross platform, multithreaded benchmark tool designed to evaluate and test database loads under a variety of system parameters.
It mainly includes the following several ways of testing:

- cpu performance
- [Disk IO performance](#)
- Scheduler performance
- Memory allocation and transmission speed
- POSIX thread performance
- Database performance (OLTP benchmark)

We are mainly concerned with io testing.

### SYSBENCH installation
###### 1. Source code
> Download source package sysbench-0.4.12.7.tar.gz 

> tar zxf sysbench-0.4.12.7.tar.gz 

> yum install gcc libtool 

> cd sysbench-0.4.12.7 

> ./configure 

> make 
###### 2. RPM
> yum install sysbench
### Parameters

```
file-num           # generates the number of test files, and defaults to 128
file-block-size    # the size of the file block during the file-block-size test
file-total-size    # the size of each file by default is 2GB
file-test-mode     # file test patterns, including seqwr (sequential writing), seqrewr (sequential reading and writing), seqrd (sequential reading), rndr, D (then read), rndwr (random write), and rndrw (random read and write)
file-io-mode       # file operation mode, synchronous or asynchronous, default is synchronization
file-fsync-all     # performs a fsync once each execution of a write, and defaults to off
```

The fileio test for sysbench requires three stages: prepare, run, and clean, and prepare is the preparation phase, generating the test files that we need, run is the actual testing phase, and cleanup is the file that the cleaning test produces.
###### --Run

```
sysbench --test=fileio --file-num=16 --file-block-size=16384 --file-total-size=2G run
```
###### --Prepare

```
sysbench --test=fileio --file-num=16 --file-block-size=16384 --file-total-size=2G prepare
```

###### --Cleanup

```
 sysbench --test=fileio --num-threads=16 --file-total-size=2G --file-test-mode=rndrw cleanup
```

## <a name="2">Benchmark Test</a>

### Setup
#### Directory Structure


```
appbenchmark
      |-------[apps]
      |        |-------[io]
      |        |       |-------fio
      |        |       |-------[sysbench]
      |        |       |        |-------ansible
      |        |       |        |-------setup.sh
      |        |       |        |-------run_test.sh
      |-------lib/ansible/roles
      |        |-------[io]
      |        |        |-------fio
      |        |        |-------[sysbench]
      |        |        |       |-------files
      |        |        |       |-------vars
      |        |        |       |-------tasks
      |-------toolset
```


###### 1. sysbench start script and test script
> [appbenchmark/apps/io/sysbench](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/sysbench)
- **[ansible](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/sysbench/ansible)** ( includeing roles and startup and test scripts )
- [test.sh](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/sysbench/run_test.sh)
- [setup.sh](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/sysbench/setup.sh) ( create a directory structure on the target machine, copy files to the target machine, install fio )

###### note
- the target machine`s ip is stored in [hosts](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/sysbench/ansible/hosts)
- the test script is in the [files](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/sysbench/files) folder



###### 2. ansible roles

> [appbenchmark/lib/ansible/roles/io/sysbench](https://github.com/open-estuary/appbenchmark/tree/lib/ansible/roles/io/fio)

- [files](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/sysbench/files) ( test scripts )
- [tasks](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/sysbench/tasks) ( install and deploy scripts )
- [vars](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/sysbench/vars)  ( system variables )

#### setup

1. enter [appbenchmark/apps/io/sysbench/ansible/hosts](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/sysbench/ansible/hosts)
1. modify the target machine ip
1. enter [appbenchmark/apps/io/sysbench](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/sysbench)
1. run sh [setup.sh](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/sysbench/setup.sh)
### Test 

1. enter [appbenchmark/apps/io/sysbench](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/sysbench)
2. run   sh [run_test.sh](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/sysbench/run_test.sh)

###### note
- After the test script starts running in the backgroud, you can view the process via [ps ux | grep ansible](#)
- The test results are stored in the [result_io](#) folder in the current user`s root directory. The result_io needs to be created by yourself.

