* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to create users and modify the corresponding authorized keys accordingly

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `local_id_rsa_pub_file`: specify the local id_rsa.pub file name
* `remote_account`: sepcify user name which will be created on remote hosts
* `remote_group`: specify user group

### Proxy configuration options

### Role Defaults

* `local_id_rsa_pub_file`: "/root/.ssh/id_rsa.pub"
* `remote_account`: "estuaryuser"
* `remote_group`: "estuaryuser"

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: hosts_file
  roles:
    - createuser

```    

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

Apache

