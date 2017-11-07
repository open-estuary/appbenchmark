* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup docker.

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured


### Role Defaults


## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: flannel_servers 

  roles:
    - docker

```    

For more examples, please refer to [flannel](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/flannel)

License
-------

Apache

