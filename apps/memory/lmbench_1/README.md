* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# Memory Benchmark Test 
## <a name="1">Introduction</a>
This is a memory test using lmbench,

1.Run "setup.sh" to install necessary softwares, copy files and create directories.

2.Run "run_test.sh" to excute test shell, include run_bw_test.sh and run_lat_test.sh.

3.After testing, the results will be saved in result_bw and result_lat directories.

## <a name="2">Benchmark Test</a>
### Test Topology

### Setup
This step is to excute install shell

Progress: setup.sh -> ./ansible/setup.yml -> roles/{tool}/task/main.yml

Running "setup.sh" will excute orders in main.yml to install softwares, copy files and create directories

### Test 
This step is to excute test shell.

Bandwidth progress : run_bw_test.sh -> ./ansible/run_bw_test.yml -> roles/{tool}/files/run_bw_test.sh

Latency progress : run_lat_test.sh -> ./ansible/run_lat_test.yml -> roles/{tool}/files/run_lat_test.sh

In memory test, there are several important orders.

1. bw_mem : test memory bandwidth(there are 9 ways : rd frd wr fwr bzero rdwr cp fcp bcopy);

2. bw_lat_rd : test memory latency; 

3. membind : bind some numa node;

4. taskset : bind thread to several CPUs;

...

notes:

In this test, you don't need to send ssh key to target machine, for the shell has help you send.

You can change target machine ip address in hosts file.

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

## <a name="3">Others</a>


