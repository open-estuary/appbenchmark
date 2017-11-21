# E-Commerce Micro-Service Solution Based on ARM64 Server
* [Introduction](#1)
* [Software Architecture](#2)
* [Setup & Deployment](#3)
* [REST API](#4)
* [Benchmark Test](#5)
* [Examples](#6)
* [Others](#7)

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

> Bebfore running Ansible, it is necessary to create new account on target servers. Then the later test could use this account to login in target servers without password.
> As for how to create new accounts automatically, please refer to [createuser.sh](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/createuser.sh)

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

> Please use LVS Virtual IP address(default to `192.168.12.100` defined in [ansible/group_vars/ARM64Server01](/apps/e-commerce-solutions/e-commerce-springcloud-microservice/ansible/group_vars/ARM64Server01)) and Port (default to `9000`) to access REST API

Method | URI  | Request JSON | Response JSON | Example
---- | ------- | ------ | ------ | -----
`GET` | /v1/order/${userid}/ |-| [OrderListRes](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [GetOrder](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIEXAMPLE.md)
`GET` | /v1/order/${userid}/${orderid} |-| [OrderRes](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [GetAllOrder](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIEXAMPLE.md)
`POST` | /v1/order/|[OrderReq](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [OrderRes](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [CreateOrder](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIEXAMPLE.md)
`DELETE`  | /v1/order/${userid}/${orderid}|-|[Stauts](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [DeleteOrder](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIEXAMPLE.md)
`GET`  | /v1/cart/${userid} |-|[CartListRes](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [GetCartByUser](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIEXAMPLE.md)
`GET`  | /v1/cart/${userid}/${cartid} |-|[CartRes](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [GetCartByCartId](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIEXAMPLE.md)
`POST` | /v1/cart|[CartReq](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [CartRes](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [CreateCart](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIEXAMPLE.md)
`POST` | /v1/cart/${userid}/${cartid}/skus/${skuid} |[Sku](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [CartRes](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [AddOrUpdateProduct](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIEXAMPLE.md)
`DELETE` | /v1/cart/${userid}/${cartid}/skus/${skuid}|-| [Status](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [DeleteProduct](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIEXAMPLE.md)
`DELETE`  | /v1/cart/${userid}/${cartid} |-|[Stauts](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [DeleteCart](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIEXAMPLE.md)
`GET`  | /v1/search?query=${query}\:*&page_size=${page_size}&page_num=${page_num}&sort=${sort}|-|[Sku](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIJSON.md) | [SearchProduct](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/RESTAPIEXAMPLE.md)

Eureka Service Example: 

<center><a href="https://github.com/open-estuary/packages/blob/master/solutions/e-commerce-springcloud-microservices/docs/e_commerce_eureka_example.png"><img src="https://github.com/open-estuary/packages/blob/master/solutions/e-commerce-springcloud-microservices/docs/e_commerce_eureka_example.png" border=0 width=1600></a></center>

## <a name="5">Benchmark Test</a>
> As for how to provision test parameters, please refer to [E_Commerce Benchmark Test Role](https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/ansible/roles/e_commerce_benchmark_test/defaults/main.yml).

Now, execute `run_test.sh` to enjoy performance test.
 * `run_test.sh`

### Test Results
> Currently the test result is NOT released to public.                                            

## <a name="6">Examples</a>
By default, it requires 12 Nodes to setup the whole system. But it is also feasible to setup the whole system as follows:

* [Setup Mini System with 4 Nodes](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/examples/mini_provision_with_4nodes)

## <a name="7">Others</a>
 * Want to clear all packages after test? Just execute `remove.sh`
