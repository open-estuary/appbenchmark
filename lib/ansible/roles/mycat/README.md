* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup mycat and to start mycat instances. 

## <a name="2">Role Variables</a>
--------------
### Expected to Be Configured

* `twemproxy_cpus`: specify the CPUs used by twemproxy.
* `twemproxy_IP`: specify the IP address of twemproxy server.
* `twemproxy_redis_config`: specify the configuration relationships for twemproxy and redis. Each instance of twemproxy can bind multiple instances of redis.
    * `twemproxy_cpu`: specify the current CPU of twemproxy.
    * `redis_ip`: spcify the IP address of binded redis-server instance.
    * `redis_port`: spcify the port of binded redis-server instance.
* `mycat_home`:
* `mycat_conf_dir`:
* `wrapper_port`:
* `mycat_serverport`:
* `mycat_managerport`:
* `user_name`:
* `password`:
* `mysql_host_M_IP`:
* `mysql_host_M_Port`:
* `host`:

### Proxy configuration options

### Role Defaults

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: twemproxy_hosts 
  remote_user: estuaryapp
  become: yes
  roles:
    - twemproxy

```    

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

Apache

