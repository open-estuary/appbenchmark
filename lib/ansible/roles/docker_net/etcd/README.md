* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup etcd which plays the role of flannel network.

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `Arch` : specify the arch of server, just arm64 or amd64.   
* `etcd_ip`: specify the ip of etcd server.  
* `ETCD_VER`: specify the version of etcd.


### Role Defaults
* `Arch`: arm64
* `etcd_ip`: 192.168.11.244
* `ETCD_VER`: v3.2.7


## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: etcd_server 

  roles:
    - etcd

```    

For more examples, please refer to [flannel](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/flannel)

License
-------

Apache

