# Setup E-Commerce Micro-Service with 4 Nodes Example
* [Deploy Software Architecture](#1)
* [Setup & Deployment](#3)
* [REST API](#4)
* [Benchmark Test](#5)
* [Others](#6)

## <a name="1">Deploy Software Architecture</a>
<center><a href="https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/examples/mini_provision_with_4nodes/estuary_e_commerce_micro_service_mini_provisioning.png"><img src="https://github.com/open-estuary/appbenchmark/blob/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice/examples/mini_provision_with_4nodes/estuary_e_commerce_micro_service_mini_provisioning.png" border=0 width=1600></a></center>

## <a name="3">Setup & Deployment</a>
> In order to deploy solution on specific server clusters, it is necessary to update `ansible/hosts` and `ansible/grouvars` accordingly. 

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
