Role Name
=========

Creates and deploys simple nginx reverse proxy configurations from supplied variables, optionally making use of the load balancing features as well.

At the time, this role does not expose all possible configurations of nginx's load balancing and proxying, but aims to use rational defaults with some configurability.

Requirements
------------

nginx with reverse proxy support installed on the server to be managed.

Role Variables
--------------

### Expected to Be Configured

* `nginx_reverse_proxy_proxies`:  list of reverse proxy configurations; each configuration needs the following variables
  * `nginx_backend_name:` string, name nginx config uses to refer to the backend
  * `nginx_nginx_domains`: list of public-facing nginx_domains to be proxied
  * `nginx_backends`: list of backend servers, including ports and [other valid parameters for `server` in the `upstream` context of an nginx config file](http://nginx.org/en/docs/http/ngx_http_upstream_module.html#server)
  * `nginx_config_name`: name to use for the proxy file (do not include the '.conf' extension, role will add this)

### Proxy configuration options

Along with the variables that must be configured for each reverse proxy configuration, some configuration options are available on a per-proxy basis:

* `nginx_balancer_config`: specify a load balancing strategy other than the default round robin. Valid options include `least_conn` (for least connections) and `ip_hash` (for session persistence using IP hashing).

See the app2proxy definition in the example playbook below.

### Role Defaults

`nginx_reverse_proxy_config_directory:` location to store config files for nginx (default: `/etc/nginx/conf.d`)
`nginx_reverse_proxy_nginx_service`: name the nginx service runs under, for the handler restart if config files are changed (default: `nginx`)

Dependencies
------------

nginx is expected to be installed on the server being managed

Example Playbook
----------------

```
---
- hosts: nginx
  remote_user: vagrant
  become: yes
  vars:
    - nginx_reverse_proxy_proxies:
      - nginx_config_name: app1proxy
        nginx_backend_name: my-backend-1
        nginx_backends:
          - localhost:1880 weight=2
          - localhost:1881
        nginx_domains:
          - app1.192.168.88.10.xip.io
      - nginx_config_name: app2proxy
        nginx_backend_name: my-backend-2
        nginx_backends:
          - localhost:1882
          - localhost:1883
        nginx_domains:
          - app2.192.168.88.10.xip.io
        nginx_balancer_config: least_conn;
      - nginx_config_name: sessionappproxy
        nginx_backend_name: session-app-backend
        nginx_backends:
          - localhost:1880
          - localhost:1881
          - localhost:1882
          - localhost:1883
        nginx_domains:
          - sessioned-app.192.168.88.10.xip.io
        nginx_balancer_config: ip_hash;

  roles:
    - ansible-nginx-reverse-proxy

```    
### Others
The simple performance status could be accessed via `http://<nginx_server>:8080/basic_status`. 

License
-------

BSD

Author Information
------------------
