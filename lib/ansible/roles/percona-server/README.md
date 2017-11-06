* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup [mysql](https://www.mysql.com/). 

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `estuaryuser` : specify the user account on the target server to run mysql
* `workspace`: specify the directory to store files and configurations during setup

### Proxy configuration options

Along with the variables that must be configured for each reverse proxy configuration, some configuration options are available on a per-proxy basis:

* `master_host`: master instance ip for slave
* `master_port`: master instance port for slave
* `master_login_name`: master instance login name for slave
* `master_login_password`: master instance login password for slave
* `cpus`: set the cpu affinity for mysql tasks
* `port`: specify the mysql listening port
* `login_name`: specify login name
* `login_password`: specify login password
* `login_host`: specify login ip
* `repl_name`: specify replcation user name
* `repl_password`: specify replication user password
* `repl_host`: specify replication user login ip


See the app2proxy definition in the example playbook below.

### Role Defaults

* `mysql_data_dir`: specify mysql databases data location
* `local_dir`: specify mysql local location
* `mysql_log_dir`: specify mysql log file location
* `mysqluser`: user to initialize and run mysql 

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: ARM64MySQLMasterHost
  remote_user: estuaryapp
  become: yes
  become_method: sudo
  vars:
     estuaryuser: estuaryapp
     workspace: /home/estuaryapp
     mysql_instance:
       - cpus: "0-30"
         port: 3306
         login_host: "{{ ansible_eth2['ipv4']['address'] }}"
         login_name: root
         login_password:
  roles:
    - estuaryrepo
    - { role: mysql-ecommerce, when: mysql_instance is defined }


```    

For more examples, please refer to [e-commerce-springcloud-microservice benchmark](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

GPL

