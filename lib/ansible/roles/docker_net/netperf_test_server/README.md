* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup server container for netperf test in flannel or weave network.

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `netarch`: specify the network of docker, weave or flannel.
* `image` : specify the image name for test.


### Role Defaults
* `netarch`: flannel
* `image`: openestuary/centos:dockernet_test


## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: test_server 

  roles:
    - netperf_test_server

```    

For more examples, please refer to [flannel](https://github.com/open-estuary/appbenchmark/tree/master/apps/docker_net/flannel)

License
-------

Apache
