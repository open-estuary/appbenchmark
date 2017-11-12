* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup Spring Cloud Zuul which plays the role of microservices gateway.

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `zuul_port`: specify the listening port of Zuul service
* `zuul_pkg_name` : specify the RPM package name of Zuul 
* `zuul_service_name`: specify the systemd service name of Zuul
* `zuul_config_dir`: specify the directory name to store Zuul configuration file (that is `application.yml`)
* `zuul_api_routes`:
  * `route_name`: specify the name of route
  * `route_path`: specify the http path whose requests will be mapped to the corresponding microservices
  * `route_serviceid`: specify the service id of microservices
* `eureka_server`: specify the server name which contain Spring Cloud Eureka server
* `eureka_port`: specify the listening port of Spring Cloud Eureka server

### Proxy configuration options

### Role Defaults
* `zuul_port`: 5555
* `zuul_pkg_name`: micro-service-api
* `zuul_service_name`: microservice-zuul
* `zuul_config_dir`: "/etc/micro-services/api-gateway"
* `zuul_api_routes`:
  * `- route_name`: api-cart
  *   route_path: "/cart/**"
  *   route_serviceid: cart-service
  * `- route_name`: api-order
  *  route_path: "/order/**"
  *  route_serviceid: order-service
  * `- route_name`: api-search
  *  route_path: "/search/**"
  * route_serviceid: search-service
* `eureka_server: localhost
* `eureka_port`: 8761
* `eureka_url`: "http://{{ eureka_server }}:{{ eureka_port }}/eureka/"

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: zuul_hosts 
  remote_user: estuaryapp
  become: yes
  roles:
    - zuul-apigateway

```    

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

Apache

