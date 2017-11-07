* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup weave network for docker.

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `main_server_ip`: specify the server ip, just one of weave servers.


### Role Defaults
* `main_server_ip`: 192.168.11.232.


## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: zuul_hosts 

  roles:
    - weave

```    

For more examples, please refer to [weave](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/weave)

License
-------

Apache

