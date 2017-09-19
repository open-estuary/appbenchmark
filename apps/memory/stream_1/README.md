* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# Memory Benchmark Test 
## <a name="1">Introduction</a>
This is a memory test using stream. Actually. stream is a subprogram of lmbench.

1.Run "setup.sh" to install necessary softwares, copy files and create directories.

2.Run "run_test.sh" to excute test shell, include run_test.sh and stream_test.sh.

3.After testing, the results will be saved in result directories.

## <a name="2">Benchmark Test</a>
### Test Topology

### Setup
This step is to excute install shell

Progress: setup.sh -> ./ansible/setup.yml -> roles/{tool}/task/main.yml

Running "setup.sh" will excute orders in main.yml to install softwares, copy files and create directories

### Test 
This step is to excute test shell.

Stream progress: run_test.sh -> ./ansible/run_test.yml -> roles/{tool}/files/run_test.sh and stream_test.sh

In memory test, there are several important orders.

1. stream : test meomory bandwidth and latency;

2. membind : bind some numa node;

3. taskset : bind thread to several CPUs;

...

notes:

In this test, you don't need to send ssh key to target machine, for the shell has help you send.

You can change target machine ip address in hosts file.

### Test Results
some test results are as follow:
*************************************************************************
stream test 

-order:

 taskset -c 0   stream -v 1 -M 200M -P 1

-result:

 STREAM copy latency: 1.40 nanoseconds

 STREAM copy bandwidth: 11425.31 MB/sec

 STREAM scale latency: 1.47 nanoseconds

 STREAM scale bandwidth: 10913.75 MB/sec

 STREAM add latency: 2.03 nanoseconds

 STREAM add bandwidth: 11844.13 MB/sec

 STREAM triad latency: 2.03 nanoseconds

 STREAM triad bandwidth: 11805.68 MB/sec

                                           

## <a name="3">Others</a>


