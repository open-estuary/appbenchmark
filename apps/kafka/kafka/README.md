* [Introduction](#1)
* [Setup](#3)

# Kafka Introduce
## <a name="1">Introduction</a>
Kafka is used for building real-time data pipelines and streaming apps. It is horizontally scalable, fault-tolerant, wicked fast.We use Kafka mainly to build a bigdata streaming system, we don't focus to do performance test now, we just to deploy and configure kafka.

### Setup
1、refer to the kafka role readme to configure varible according to your system；
2、modify hosts file in apps/kafka/kafka/ansible to include your hosts, be careful about your cpu platform.
3、run setup.sh file in apps/kafka/kafka
4、optional step, if you want to remove kafka, please configure the run_mode in lib/ansible/roles/kafka/defaults/main.yml.
