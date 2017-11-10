# E-Commerce Micro-Service Solution Based on ARM64 Server
* [Introduction](#1)
* [Software Architecture](#2)
* [Setup & Deployment](#3)
* [REST API](#4)
* [Benchmark Test](#5)

## <a name="1">Introduction</a>

It is to demonstrate one e-commerce microservice solution with high performance、high scalability、high availability and high reliability based on ARM64 servers. 
In addition, it uses many middlewares provided by Spring Cloud such as:  
  * `Eureka`: MicroService Service Discovery  
  * `Zuul`: MicroService Api-Gateway  
  * `Hystrix, Feign, Ribbon, Zipkin, ...`

## <a name="2">Software Architecture</a>
<center><a href="https://github.com/open-estuary/packages/blob/master/solutions/e-commerce-springcloud-microservices/docs/estuary_e_commerce_micro_service_software_architecture.png"><img src="https://github.com/open-estuary/packages/blob/master/solutions/e-commerce-springcloud-microservices/docs/estuary_e_commerce_micro_service_software_architecture.png" border=0 width=1600></a></center>

## <a name="3">Setup & Deployment</a>
> In order to deploy solution on specific server clusters, it is necessary to update `ansible/hosts` and `ansible/grouvars` accordingly. 

### Mount Devices to Specified Path
If it is necessary to mount new devices to specified target paths, please run `mount_devices.sh` accordingly.  
In addition, it should specify the corresponding devices and target paths in [ansible/groupvars/all](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/ansible/group_vars/all).
For more details, please refer to `mount` [role](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/mount).
 * execute `mount_devices.sh`

### Setup including provisioning Applications 
It is necessary to update some configurations based on real servers, such as:
 * [host](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/ansible/hosts): Specify IP address of servers
 * [site_arm64.yml](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/ansible/site_arm64.yml), site_xxx.yml: Specify how to deploy applications on different servers
 * [groupvars](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/ansible/group_vars): Define specific variables for current provisioning. As for how to setup each variable, please refer to the `README.md` of each [role](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/ansible/roles)

Then just execute `setup.sh` to deploy the whole e-commerce solution.
 * `setup.sh` :

### Load Test Data 
Before executing test, it is necessary to import data as follows:
 * `run_loaddata.sh`:

## <a name="4">REST API</a>
E-Commerce MicroService REST API

> Please use LVS Virtual IP address(default to `192.168.11.100`) and Port (default to `9000`) to access REST API

Method | URI  | Request JSON | Response JSON
---- | ------- | ------ | -----
`GET` | /v1/orders/${orderid} |-| [OrderRes](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md)
`POST` | /v1/orders/|[OrderReq](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [Status](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md)
`DELETE`  | /v1/orders/${userId}/${orderid}|-|[Stauts](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md)
`GET`  | /v1/carts/{userid}/${cartid} |-|[CartRes](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md)
`POST` | /v1/carts/${userid}/${cartid}|[CartReq](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [Status](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md)
`POST` | /v1/carts/${userid}/${cartid}/skus/${skuid} |[CartReq](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [Status](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md)
`DELETE` | /v1/carts/${userid}/${cartid}/skus/${skuid}|-| [Status](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md)
`DELETE`  | /v1/carts/{userid}/${cartid} |-|[Stauts](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md)
`GET`  | /v1/search?query=*:*&page_size={page_size}&page_num={page_num}&sort={sort}|-|[Sku](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md)

Eureka Service Example: 

<center><a href="https://github.com/open-estuary/packages/blob/master/solutions/e-commerce-springcloud-microservices/docs/e_commerce_eureka_example.png"><img src="https://github.com/open-estuary/packages/blob/master/solutions/e-commerce-springcloud-microservices/docs/e_commerce_eureka_example.png" border=0 width=1600></a></center>

## <a name="4">Benchmark Test</a>
 * `run_test.sh`

### Test Topology

### Test Results
                                           


