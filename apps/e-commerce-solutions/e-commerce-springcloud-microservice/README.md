# E-Commerce Micro-Service Solution Based on ARM64 Server
* [Introduction](#1)
* [Software Architecture](#2)
* [Setup & Deployment](#3)
* [Benchmark Test](#4)

## <a name="1">Introduction</a>

It is to demonstrate one e-commerce microservice solution with high performance、high scalability、high availability and high reliability based on ARM64 servers. 
In addition, it uses many middlewares provided by Spring Cloud such as:  
  * `Eureka`: MicroService Service Discovery  
  * `Zuul`: MicroService Api-Gateway  
  * `Hystrix, Feign, Ribbon, Zipkin, ...`

## <a name="2">Software Architecture</a>
<center><a href="docs/estuary_e_commerce_micro_service_software_architecture.png"><img src="https://github.com/open-estuary/packages/blob/master/solutions/e-commerce-springcloud-microservices/docs/estuary_e_commerce_micro_service_software_architecture.png" border=0 width=1600></a></center>

## <a name="3">Setup & Deployment</a>
> In order to deploy solution on specific server clusters, it is necessary to update `ansible/hosts` and `ansible/grouvars` accordingly. 

 * `setup.sh` :
 * `run_loaddata.sh`:
## <a name="4">Benchmark Test</a>
 * `run_test.sh`
### Test Topology

### Test Results
                                           


