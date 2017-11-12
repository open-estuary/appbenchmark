* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup Spring Cloud Eureka which plays the role of microservices gateway.

## <a name="2">Role Variables</a>
--------------
### Expected to Be Configured

* `eureka_port`: specify the listening port of Eureka service
* `eureka_pkg_name` : specify the RPM package name of Eureka 
* `eureka_service_name`: specify the systemd service name of Eureka
* `eureka_config_dir`: specify the directory name to store Eureka configuration file (that is `application.yml`)
* `eureka_server`: specify the server name which contain Spring Cloud Eureka server
* `eureka_port`: specify the listening port of Spring Cloud Eureka server

### Proxy configuration options

### Role Defaults
* `eureka_port`: 8761
* `eureka_pkg_name`: micro-service-api
* `eureka_service_name`: microservice-eureka
* `eureka_config_dir`: "/etc/micro-services/api-gateway"
* `eureka_server`: localhost
* `eureka_url`: "http://{{ eureka_server }}:{{ eureka_port }}/eureka/"

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: eureka_hosts 
  remote_user: estuaryapp
  become: yes
  roles:
    - eureka-service-discovery

```    

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

Apache

