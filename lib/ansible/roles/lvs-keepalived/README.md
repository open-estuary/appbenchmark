* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to setup LVS+ keepalived for load balancing.

## <a name="2">Role Variables</a>
--------------
### Expected to Be Configured

### Proxy configuration options

### Role Defaults

## <a name="3">Example Playbook</a>
----------------

```
---
- name: Install LVS+KeepAlived on ARM64Server
  hosts: WebServerD05
  remote_user: estuaryapp
  become: yes
  become_method: sudo
  roles:
    - role: lvs-keepalived
      global_defs:
        - notification_email: 'sjtuhjh@hotmail.com'
      vrrp_instances:
        - name: 'VI_01'
          state: 'MASTER'
          interface: 'enp2s0f1'
          virtual_router_id: 10
          priority: 100
          auth_type: 'PASS'
          auth_pass: 'secpass'
          virtual_ipaddress:
            - '192.168.111.1'
            - '192.168.111.2'
      virtual_servers:
        - name: '192.168.111 80'
          delay_loop: 6
          lb_algo: 'rr'
          lb_kind: 'NAT'
          protocol: 'TCP'
          alpha: True
          omega: True
          quorum: 1
          hysteresis: 0
          persistence_timeout: 60
          real_servers: 
            - address: '192.168.11.16'
              port: 9000
              weight: 100
              check:
                type: "TCP_CHECK"
                connect_port: 9000
                connect_timeout: 15
                nb_get_retry: 5
                delay_before_retry: 3

```    

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice) and [site.yml example](https://github.com/open-estuary/appbenchmark/blob/master/lib/ansible/roles/lvs-keepalived/site.yml.example)

License
-------

Apache

