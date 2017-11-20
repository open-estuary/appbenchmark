* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup speccpu2006 which is a popular cpubenchmark tool.

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `hosts`: specify the remote hosts you want to do cpu test.

### group varibles
* `estuaryuser`: estuaryapp
* `workspace`: /home/estuaryapp

## <a name="3">Example Playbook</a>
----------------

```
- name: Install Speccpu2006 on ARM64Server
  hosts: CpuTestD05
  roles:
    - spec_cpu2006

```    

For more examples, please refer to https://github.com/open-estuary/appbenchmark/tree/master/apps/cpu/spec_cpu2006

License
-------

Apache

