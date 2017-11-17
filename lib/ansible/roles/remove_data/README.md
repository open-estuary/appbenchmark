* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to delete mysql and redis data. 

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `estuaryuser` : specify the user account on the target server to run mysql
* `workspace`: specify the directory to store files and configurations during setup

### Proxy configuration options

Along with the variables that must be configured for each reverse proxy configuration, some configuration options are available on a per-proxy basis:

* `cpus`: set the cpu affinity for mysql tasks
* `login_port`: specify the mysql listening port
* `login_name`: specify login name
* `login_password`: specify login password
* `login_host`: specify login ip

See the app2proxy definition in the example playbook below.

### Role Defaults

* `sql_location`: specify sql file location
* `mysqluser`: user to initialize and run mysql 

## <a name="3">Example Playbook</a>
----------------

```
- hosts: ARM64MySQLCartDATA
  remote_user: estuaryapp
  become: yes
  become_method: sudo
  vars:
     database: e-commerce-cart
     sql_location: /opt/data/
     listen_port: 3306
     login_host: "{{ ansible_eth2['ipv4']['address'] }}"
     login_name: root
     login_password:
     remove_data: remove_data.sql
  roles:
    - { role: remove_data }

```    

For more examples, please refer to [e-commerce-springcloud-microservice benchmark](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

GPL

