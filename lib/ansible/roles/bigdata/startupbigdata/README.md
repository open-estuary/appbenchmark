* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup Spring Cloud Zuul which plays the role of microservices gateway.

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured
master_machine:
  hadoop: 192.168.1.42
  spark: 192.168.1.42

* `master_machine`:
  * `hadoop`: specify the hostname of hadoop master
  * `spark`: specify the hostname of spark master

### Proxy configuration options

## <a name="3">Example Playbook</a>
----------------

```
---
- name: startup bigdata services
  hosts: BigDataMachines
  gather_facts: yes
  vars:
    tmpworkspace: /tmp
    workspace: /home/BigData
  vars_files:
    - ./roles/DeployBigData/defaults/main.yml
    - ./roles/DeployBigData/vars/main.yml
  roles:
    - startupbigdata

```    


License
-------

Apache

