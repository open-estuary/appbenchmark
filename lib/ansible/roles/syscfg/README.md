* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to set system limit values and kernel value via `/etc/sysctl.conf`.

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured


### Proxy configuration options


### Role Defaults


## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: solr_hosts 
  remote_user: estuaryapp
  become: yes
  roles:
    - syscfg

```    

License
-------

Apache

