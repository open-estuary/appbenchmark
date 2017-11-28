* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to Deploy BigData to remote machines.

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `ZK_serverlist`: specify the zookeeper host list in the way: hostname1:port1-1:port1-2,hostname2:port2-1:port2-2...
* `spark_driver_memory` : specify the application driver used memory
* `master_machine`: dictionary structure, you should list the machine master of hadoop and spark
* `slaves_machine`: dictionary structure, you should list the machine slaves of hadoop and spark

### Proxy configuration options

### Role Defaults
* `ZK_clientPort`: 2181
* `ZK_dataLogDir`: /var/zookeeperTest/datalogdir
* `ZK_dataDir`: /var/zookeeperTest/datadir
* `ZK_tickTime`: 2000
* `ZK_initLimit`: 4
* `ZK_syncLimit`: 2
* `apache_mirror`: "http://mirrors.tuna.tsinghua.edu.cn/apache"
* `scala_download_mirror`: "https://downloads.lightbend.com/scala"
* `java_download_mirror`: "http://download.oracle.com/otn-pub/java/jdk/8u151-b12/e758a0de34e24606bca991d704f6dcbf"
* `hadoop_tmp_dir`: "{{SOFTWARE_INSTALL_PATH}}/hadoop-{{HADOOP_VERSION}}/tmp"
* `dfs_namenode_name_dir`: "{{SOFTWARE_INSTALL_PATH}}/hadoop-{{HADOOP_VERSION}}/dfs/name"
* `dfs_datanode_data_dir`: "{{SOFTWARE_INSTALL_PATH}}/hadoop-{{HADOOP_VERSION}}/dfs/data"


## <a name="3">Example Playbook</a>
----------------

```
---

- name: install bigdata on remote x86 machines
  hosts: Install_BigDataMachines_X86
  gather_facts: yes
  roles:
    - DeployBigData

- name: install bigdata on remote hosts
  hosts: Install_BigDataMachines_Arm64
  gather_facts: yes
  roles:
    - estuaryrepo
    - DeployBigData

```    

## <a name="4">Additional specification</a>
* you may need to change the hadoop mirror site, for the site is invalidate usually
* bigdata need to deploy kafka, it's in site "https://github.com/open-estuary/appbenchmark/tree/master/apps/kafka/kafka", please refer this page to deploy kafka

License
-------

Apache

