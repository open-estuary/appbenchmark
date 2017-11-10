* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup Spring Cloud Search which plays the role of e-commerce search.

## <a name="2">Role Variables</a>
--------------
### Expected to Be Configured

* `search_port`: specify the listening port of Search service
* `eureka_server`: specify the server name which contain Spring Cloud Eureka server
* `eureka_port`: specify the listening port of Spring Cloud Eureka server
* `twemproxy_ip`: specify the ip address of Twemproxy
* `twemproxy_port`: specify the listening port of Twemproxy
* `solr_ip`: specify the ip address of Solr
* `solr_port`: specify the listening port of Solr
* `zipkin_server`: specify the ip address of Zipkin
* `zipkin_port`: specify the listening port of Zipkin
* `search_pkg_name`: specify the RPM package name of Search
* `search_config_dir`: specify the directory name to store Search configuration file (that is `application.yml`)


### Proxy configuration options

### Role Defaults
* `search_port`: 8002
* `eureka_server`: 192.168.11.233
* `eureka_port`: 8761
* `twemproxy_ip`: 192.168.11.232
* `twemproxy_port`: 22146
* `solr_ip`: 192.168.11.232
* `solr_port`: 8983
* `zipkin_server`: 192.168.11.225
* `zipkin_port`: 
* `search_pkg_name`: micro-service-search
* `search_config_dir`: "/etc/e-commerce/search"
* `eureka_url`: "http://{{ eureka_server }}:{{ eureka_port }}/eureka/"

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: search_hosts
  remote_user: estuaryapp
  become: yes
  roles:
    - search

```

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

Apache
