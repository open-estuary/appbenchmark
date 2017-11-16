* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to import data to [redis](https://redis.io/). 

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `estuaryuser` : specify the user account on the target server to run mysql
* `redis_import_data`: specify the redis configurations that you need import data to.
   * `IP_address `: specify the IP address of redis
   * `port`: specify the port of redis

### Proxy configuration options

### Role Defaults

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: ARM64REDISSEARCHDATA
  remote_user: estuaryapp
  become: yes
  become_method: sudo
  roles:
    - estuaryrepo
    - redis-data


```    

For more examples, please refer to [e-commerce-springcloud-microservice benchmark](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

GPL

