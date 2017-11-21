* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup kafka .

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `kafka_listen_address`: defines a specifc address for kafka to listen on, by defaults listens on all interfaces
* `kafka_id` : Id to be used if one can't or shouldn't be derived from kafka_hosts. This will happen if kafka_hosts doesn't contain the fqdn but an alias 
* `monasca_log_level`: Log level to be used for Kafka logs. Defaults to WARN
* `monasca_wait_for_period`: The time in seconds for how long to wait for Kafka's port to be available after starting it. Default is 30 seconds.
* `run_mode`: One of Deploy, Stop, Install, Start, or Use. The default is Deploy which will do Install, Configure, then Start.
* `zookeeper_host`: specify the zookeeper host and port


### Proxy configuration options

### Role Defaults
* `apache_mirror`: http://mirrors.tuna.tsinghua.edu.cn/apache
* `kafka_hosts`: "{{ansible_fqdn}}:9092"
* `kafka_version`: 0.11.0.1
* `scala_version`: 2.11
* `run_mode`: Deploy
* `skip_install`: False
* `kafka_num_network_threads`: 2
* `kafka_num_io_threads`: 2
* `kafka_num_partitions`: 2
* `kafka_port`: 9092
* `kafka_socket_send_buffer_bytes`: 1048576
* `kafka_socket_receive_buffer_bytes`: 1048576
* `kafka_socket_request_max_bytes`: 1048576000
* `kafka_log_flush_interval_messages`: 10000
* `kafka_log_flush_interval_ms`: 1000
* `kafka_log_retention_bytes`: 1048576000  #1000 M
* `kafka_log_retention_hours`: 24
* `kafka_log_segment_bytes`: "{{kafka_log_retention_bytes}}"
* `kafka_log_cleanup_interval_mins`: 1
* `kafka_heap_opts`: "-Xmx10G -Xms10G"
* `kafka_tarball_location`: "{{ workspace }}"
* `kafka_tarball_name`: "kafka_{{ scala_version }}-{{kafka_version}}.tgz"
* `monasca_wait_for_period`: 30
* `monasca_log_level`: WARN
* `zookeeper_connection_timeout_ms`: 1000000

## <a name="3">Example Playbook</a>
----------------

```
---
- name: Install kafka on remote X86 hosts
  hosts: Kafka_X86_Hosts
  roles:
    - kafka


- name: Install kafka on remote D05 hosts
  hosts: Kafka_D05_Hosts
  roles:
    - estuaryrepo
    - kafka

```    

For more examples, please refer to (https://github.com/open-estuary/appbenchmark/tree/master/apps/kafka)

License
-------

Apache

