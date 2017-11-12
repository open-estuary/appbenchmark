* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup Performance Co-Pilot(PCP) which is an open source, distributed, metrics gathering and anlysis system.

## <a name="2">Role Variables</a>
--------------

Usually each server will run `pmcd` (that is `pcp_pmcd`) while only one client server will run one web server (that is `pcp_web_vector`).
In addition, users could check each server's performance via this web server.

### Expected to Be Configured

* `pcp_pmcd`: specify whether it is to start pmcd daemon
* `pcp_web_vector`: specify whether it is to start web server 

### Proxy configuration options
* `pcp_cpus`: specify the cpu affinity for pmcd daemon

### Role Defaults
* `pcp_pmcd`: true
* `pcp_web_vector`: false

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: pcp_hosts 
  remote_user: estuaryapp
  become: yes
  roles:
    - pcp

```    

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

Apache

