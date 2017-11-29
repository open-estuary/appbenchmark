* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup hibench which is a bigdata test suits.

## <a name="2">Role Variables</a>
--------------

### Role Defaults
* `apache_mirror_site`: "http://mirrors.tuna.tsinghua.edu.cn/apache"
* `maven_bigversion`: 3
* `maven_version`: "{{maven_bigversion}}.3.9"
* `maven_tarball_name`: "apache-maven-{{maven_version}}-bin.tar.gz"
* `maven_install_location`: /opt
* `hibench_build_option`: all
* `scala_default_version`: 2.11
* `spark_default_version`: 2.2

## <a name="3">Example Playbook</a>
----------------

```
---
- name: build hibench on remote machine
  hosts: Build_Hibench
  gather_facts: no
  roles:
    - DeployHiBench

```    

License
-------

Apache

