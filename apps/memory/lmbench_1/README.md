* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# Memory Benchmark Test 
## <a name="1">Introduction</a>
this is a test about memory.in this case, i user two ways to measure the performance of memory.The one is lmbench, and the other one is stream.Actually, stream is a subcommand of lmbench.
There are two steps.Firstly, you should run "setup.sh" which can help you to install some neccessary softwares, copy some files and create some directories.Secondly, you should 
run "run_test.sh"(in lmbench, you should run "run_bw_test.sh and run_lat_test.sh") which can help you to excute test shell. After a period of time, you can get test results.These results
will be create at target machine, but don't worry, finally, these results will be send to your host machine.

## <a name="2">Benchmark Test</a>
### Test Topology

### Setup
setup.sh -> ./ansible/setup.yml -> roles/{tool}/task/main.yml
you should run "setup.sh" firstly, then it will call ./ansible/setup.yml, and the setup.yml will call ./roles/{tool}/task/main.yml. So, running "setup.sh" will excute orders in main.yml.
The file is aimed at installing softwares, copying files, creating directory.

### Test 
run_bw_test.sh -> ./ansible/run_bw_test.yml -> roles/{tool}/files/run_bw_test.sh
run_lat_test.sh -> ./ansible/run_lat_test.yml -> roles/{tool}/files/run_lat_test.sh
run_test.sh -> ./ansible/run_test.yml -> roles/{tool}/files/run_test.sh and stream_test.sh

This step is to excute test shell.The process is above.In memory test, there are several important orders.Let me introduce briefly.
1. bw_mem : test memory bandwidth(there are 9 ways : rd frd wr fwr bzero rdwr cp fcp bcopy);
2. bw_lat_rd : test memory latency; 
3. stream : test meomory bandwidth and latency;
4. membind : bind some numa node.
...

notes:

In this test, you don't need to send ssh key to target machine, for the shell has help you send.
You can change target machine ip address in hosts file. It allows several ip address to work together.

### Test Results
some test results are as follow:
*************************************************************************
bandwidth test
-order:
 for bm in rd frd wr fwr bzero rdwr cp fcp bcopy
     do 
      nohup numactl --localalloc -C 0 bw_mem -P 1 -N 5 32M $bm
 done

-result
32.00 9516.73

32.00 7842.18

32.00 7689.54

32.00 8933.56
32.00 8951.05
32.00 5920.44
32.00 5223.64
32.00 6776.79
32.00 7170.87
the left data is size(MB), the right data is bandwidth(MB)

************************************************************************
latency test
-order:
 numactl --localalloc --physcpubind=0 lat_mem_rd -P 1 -t 256M 128

-result
"stride=128
0.00049 1.667
0.00098 1.667
0.00195 1.667
0.00293 1.667
0.00391 1.667
0.00586 1.667
0.00781 1.667
0.01172 1.667
0.01562 1.667
0.02344 7.506
0.03125 6.133
0.04688 5.805
0.06250 7.507
0.09375 7.521
0.12500 7.508
0.18750 9.847
0.25000 10.198
0.37500 10.068
0.50000 10.100
0.75000 13.300
1.00000 17.250
1.50000 21.901
2.00000 23.286
3.00000 24.578
4.00000 25.233
6.00000 27.642
8.00000 35.726
12.00000 62.295
16.00000 81.065
24.00000 108.238
32.00000 120.291
48.00000 126.411
64.00000 128.708
96.00000 128.736
128.00000 130.194
192.00000 130.870
256.00000 135.272

the left data is size(MB) , the right is latency(ns)

***********************************************************************
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

