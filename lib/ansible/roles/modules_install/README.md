* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to install/uninstall packages.  

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `rpm_packages_list`:
* `deb_packages_list`:
* `packages_state`: Whether to install (`present` or `installed`, `latest`) or remove (`absent` or `removed`) packages

### Proxy configuration options

### Role Defaults

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: install_hosts 
  remote_user: estuaryapp
  become: yes
  vars:
    rpm_packages_list: "ethtool"
    packages_state: "present"
  roles:
    - modules_install

```    

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

Apache

