* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# CPU Benchmark Test 
## <a name="1">Introduction</a>
  This project focus on cpu performance test. It supply spec cpu2006 cpu test tool, deployment and test method. In this test we focus on INT and FLOAT calculation. Generally, cpu performance test include two important part, one is speed test, the other is rate test. Speed test is to test the calculating speed of single-task. Rate test is aimed at testing the throughput capacity of. The two mode have their own Base value and Peak value. In base test, we limit using only one compiler and compile optimizations no larger than four. In peak test we can using different compiler and compile optimizations in different projects in order to maxium test results.
## <a name="2">Benchmark Test</a>
### Test Topology
In this test, we use two servers, one is for ansible master, the other for deployment of cpu test tool. Our speccpu2006 tool is on remote boardserver and is being compiled. Firstly, our ansible master pull the speccpu2006 tool package from remote boardserver. Secondly, ansible master connect to deploy host, and push speccpu2006 package and some necessary script to deploy host. Thirdly, you can run ansible test yml file to execute cpu test.
### Setup
1、config the connection of ansible master and deployment hosts；
2、modify hosts file in apps/cpu/spec_cpu2006/ansible to include your test hosts.
3、run setup.sh file in apps/cpu/spec_cpu2006.


### Test 
1、run run_test.sh file in apps/cpu/spec_cpu2006.

### Test Results
Test results is in /home/estuaryapp/speccpu2006/result directory in each host.
## <a name="3">Others</a>
little optimization:</br>
1、test result should be fetched by ansible master.</br>
2、add a script to config the connection of ansible master and deployment hosts.</br>
