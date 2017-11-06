* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup Redis and to start redis-server instances.

## <a name="2">Role Variables</a>
--------------
### Expected to Be Configured

* `redis_cpus`: specify the CPUs used by redis-server

### Proxy configuration options

### Role Defaults

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: redis_hosts 
  remote_user: estuaryapp
  become: yes
  roles:
    - redis

```    

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

BSD

