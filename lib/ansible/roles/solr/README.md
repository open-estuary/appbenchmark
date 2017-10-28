Role Name
=========

This ansible role is to setup [solr](http://lucene.apache.org/solr/). 

Role Variables
--------------

### Expected to Be Configured

* `estuaryuser` : specify the user account on the target server to run solr
* `workspace`: specify the directory to store files and configurations during setup

### Proxy configuration options

Along with the variables that must be configured for each reverse proxy configuration, some configuration options are available on a per-proxy basis:

* `solr_host`: specify the solr hostname exposed to cluster state
* `solr_jmx_enable`: specify whether the Solr JMX functionality is enabled or not
* `solr_jmx_port`: the Solr JMX Port 
* `solr_memory`: specify the JAVA heap size for solr
* `solr_cpus`: set the cpu affinity for solr tasks
* `solr_gc_tune`: specify JVM options 
* `solr_port`: specify the solr listening port

See the app2proxy definition in the example playbook below.

### Role Defaults

* `solr_version`: 6.6.1
* `solr_memory`: 32g
* `solr_port`: 8983
Example Playbook
----------------

```
---
- hosts: solr_hosts 
  remote_user: estuaryapp
  become: yes
  vars:
    - estuaryuser: estuaryapp
    - workspace: /home/estuaryapp
    - solr_port: 8983
    - solr_gc_tune: "-XX:+HeapDumpOnOutOfMemoryError -XX:HeapDumpPath=/opt/solr/server/logs/solr.oomdump \
        -XX:G1ReservePercent=20 -XX:ConcGCThreads=32 -XX:ParallelGCThreads=32  \
        -XX:+PerfDisableSharedMem -XX:+UseLargePages -XX:+AggressiveOpts \
        -XX:+ParallelRefProcEnabled -XX:InitiatingHeapOccupancyPercent=75 \
        -XX:-OmitStackTraceInFastThrow  \
	-XX:+UseG1GC -XX:MaxGCPauseMillis=250 -XX:+UnlockExperimentalVMOptions -XX:G1HeapRegionSize=32M"
    - solr_cpus: "2-63"
  roles:
    - solr

```    

For more examples, please refer to [solr benchmark](https://github.com/open-estuary/appbenchmark/tree/master/apps/solr)

License
-------

Apache

