* [Introduction](#1)
* [Benchmark Test](#2)
* [Others](#3)

# Bigdata Benchmark
## <a name="1">Introduction</a>
　This project focus on bigdata including deploy/startup/start/stop, and its test tool suit deploy and build. Bigdata system is composed of hadoop、spark、kafka、zookeeper four parts. kafka is not in this project, you can refer it in "https://github.com/open-estuary/appbenchmark/tree/master/apps/kafka/kafka". We mainly to do Spark Streaming test. From this perspective, hibench is a test tools and will general stream to kafka, just like a producer of kafka. Kafka will store the stream in local log files. Spark will fetch the stream from kafka, just like a consumer of kafka, and process the stream.
## <a name="2">Benchmark Test</a>
### Test Topology
　In this test, we use at least three machines. For example, we use three nodes. In the three nodes, we deploy hadoop、spark、kafka、zookeeper cluster.

### Work loads

1. Identity (identity)

    This workload reads input data from Kafka and then writes result to Kafka immediately, there is no complex business logic involved.

2. Repartition (repartition)

    This workload reads input data from Kafka and changes the level of parallelism by creating more or fewer partitionstests. It tests the efficiency of data shuffle in the streaming frameworks.
    
3. Stateful Wordcount (wordcount)

    This workload counts words cumulatively received from Kafka every few seconds. This tests the stateful operator performance and Checkpoint/Acker cost in the streaming frameworks.
    
4. Fixwindow (fixwindow)

    The workloads performs a window based aggregation. It tests the performance of window operation in the streaming frameworks.
 
### Setup
1、config the connection of ansible master and deployment hosts；
2、run setup.sh file in apps/bigdata/bigdata.
3、run startup.sh when you first start bigdata system, or start.sh when you have started ever.
4、run stop.sh to stop the bigdata service.
5、run build_hibench.sh to deploy hibench environment.


### Test 
* we do not supply test script, because there are four workloads, this is streaming applcation, we don't know when to stop the test.so you should modify configure in hibench, and test it by hand.

### Test Results
About test result, please refer to dbox. Its sit address is: 海思/Estuary/Docs/V500/02.Development/2.8.ST
## <a name="3">Others</a>
little optimization:</br>
1、add a script to config the connection of ansible master and deployment hosts.</br>
