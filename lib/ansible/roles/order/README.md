* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup Spring Cloud Order which plays the role of e-commerce order.

## <a name="2">Role Variables</a>
--------------
### Expected to Be Configured

* `order_port`: specify the listening port of Order service
* `eureka_server`: specify the server name which contain Spring Cloud Eureka server
* `eureka_port`: specify the listening port of Spring Cloud Eureka server
* `mycat_ip`: specify the ip address of MyCat
* `mycat_port`: specify the listening port of MyCat
* `database_name`: specify the name of Database
* `twemproxy_ip`: specify the ip address of Twemproxy
* `twemproxy_port`: specify the listening port of Twemproxy
* `zipkin_server`: specify the ip address of Zipkin
* `zipkin_port`: specify the listening port of Zipkin
* `order_pkg_name`: specify the RPM package name of Order
* `order_config_dir`: specify the directory name to store Order configuration file (that is `application.yml`)


### Proxy configuration options

### Role Defaults
* `order_port`: 8000
* `eureka_server`: 192.168.11.233
* `eureka_port`: 8761
* `mycat_ip`: 192.168.11.218
* `mycat_port`: 8066, 8067
* `database_name`: e-commerce-order
* `twemproxy_ip`: 192.168.11.232
* `twemproxy_port`: 22145
* `zipkin_server`: 192.168.11.225
* `zipkin_port`: 
* `order_pkg_name`: micro-service-order
* `order_config_dir`: "/etc/e-commerce/order"
* `eureka_url`: "http://{{ eureka_server }}:{{ eureka_port }}/eureka/"

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: order_hosts
  remote_user: estuaryapp
  become: yes
  roles:
    - order

```

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

Apache

