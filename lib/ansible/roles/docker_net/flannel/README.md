* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup flannel network for docker.

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `Arch`: specify the arch of server, just arm64 or amd64.
* `Etcd_Server` : specify etcd server ip. 
* `Netcard`: specify the netcard for flannel.
* `FLANNEL_VER`: specify the version of flannel.


### Role Defaults
* `Arch`: arm64.
* `Etcd_Server` : 192.168.11.244.
* `Netcard`: eth2.
* `FLANNEL_VER`: v0.9.0.

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: flannel_servers 

  roles:
    - flannel

```    

For more examples, please refer to [flannel](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/flannel)

License
-------

Apache

