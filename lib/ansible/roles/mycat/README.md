* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup mycat and to start mycat instances. 

## <a name="2">Role Variables</a>
--------------
### Expected to Be Configured

* `mycat_1_cpus`: specify the CPUs of mycat.
* `mycat_2_cpus`: specify the CPUs of mycat. If there is only 1 mycat instance in enviroment, this parameter does not need to be configured.
* `mycat_home`: the home directory of mycat.
* `mycat_conf_dir`: the config directory of mycat.
* `wrapper_port`:  specify the port of wrapper.
* `mycat_serverport`: specify the server port of mycat.
* `mycat_managerport`: specify the manager port of mycat.

* `mysql_cfg`:  specify the configuration relationships for mycat and mysql. Each instance of mycat can bind multiple databases of mysql.
    * `user_name`: spccify the user name of database.
    * `password`: spccify the password of database.
    * `mysql_host_M_IP`: spcify the IP address of master database.
    * `mysql_host_M_Port`: spcify the port of master database.

* `mysql_slave_hosts`: specify the configurations of slave hosts.
    * `host`: specify name of slave host.
    * `mysql_host_S_IP`: specify the IP address of slave host.
    * `mysql_host_S_Port`: specify the port of slave host.

### Proxy configuration options

### Role Defaults
* `mycat_home`: "~/mycat_home"
* `mycat_conf_dir`: "~/mycat_home/src/mycat/conf"
* `wrapper_port`: 1984
* `mycat_serverport`: 8066
* `mycat_managerport`: 9066
* `user_name`: "root"
* `password`: ""

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: mycat_hosts 
  remote_user: estuaryapp
  become: yes
  roles:
    - mycat,
      mycat_home: "~/mycat_home",
        mycat_conf_dir: "~/mycat_home/src/mycat/conf",
        wrapper_port: 1984,
        mycat_serverport: 8066,
        mycat_managerport: 9066,
        when: mycat_1_cpus is defined }
```    

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------


