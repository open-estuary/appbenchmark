* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup Spring Cloud Cart which plays the role of e-commerce cart.

## <a name="2">Role Variables</a>
--------------
### Expected to Be Configured

* `cart_port`: specify the listening port of Cart service
* `eureka_server`: specify the server name which contain Spring Cloud Eureka server
* `eureka_port`: specify the listening port of Spring Cloud Eureka server
* `mycat_ip`: specify the ip address of MyCat
* `mycat_port`: specify the listening port of MyCat
* `database_name`: specify the name of Database
* `twemproxy_ip`: specify the ip address of Twemproxy
* `twemproxy_port`: specify the listening port of Twemproxy
* `zipkin_server`: specify the ip address of Zipkin
* `zipkin_port`: specify the listening port of Zipkin
* `cart_pkg_name`: specify the RPM package name of Cart
* `cart_config_dir`: specify the directory name to store Cart configuration file (that is `application.yml`)


### Proxy configuration options

### Role Defaults
* `cart_port`: 8001
* `eureka_server`: 192.168.11.233
* `eureka_port`: 8761
* `mycat_ip`: 192.168.11.219
* `mycat_port`: 8066, 8067
* `database_name`: e-commerce-cart
* `twemproxy_ip`: 192.168.11.219
* `twemproxy_port`: 22135
* `zipkin_server`: 192.168.11.225
* `zipkin_port`: 
* `cart_pkg_name`: micro-service-cart
* `cart_config_dir`: "/etc/e-commerce/cart"
* `eureka_url`: "http://{{ eureka_server }}:{{ eureka_port }}/eureka/"

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: cart_hosts
  remote_user: estuaryapp
  become: yes
  roles:
    - cart

```

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

Apache

