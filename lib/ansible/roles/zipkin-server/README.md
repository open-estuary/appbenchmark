* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup Spring Cloud Zuul which plays the role of microservices gateway.

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `zipkin_port`: specify the listening port of Zuul service
* `zipkin_pkg_name` : specify the RPM package name of Zuul 
* `zipkin_service_name`: specify the systemd service name of Zuul
* `zipkin_config_dir`: specify the directory name to store Zuul configuration file (that is `application.yml`)

* `eureka_server`: specify the server name which contain Spring Cloud Eureka server
* `eureka_port`: specify the listening port of Spring Cloud Eureka server

### Proxy configuration options

### Role Defaults
* `zipkin_port`: 5555
* `zipkin_pkg_name`: micro-service-api
* `zipkin_service_name`: micro-service-zipkin
* `zipkin_config_dir`: "/etc/micro-services/zipkin"

* `eureka_server: localhost
* `eureka_port`: 8761
* `eureka_url`: "http://{{ eureka_server }}:{{ eureka_port }}/eureka/"

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: zipkin_hosts 
  remote_user: estuaryapp
  become: yes
  roles:
    - zipkin-server

```    

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

Apache

