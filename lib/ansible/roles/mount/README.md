* [Introduction](#1)
* [Role Variables](#2)
* [Example Playbook](#3)

## <a name="1">Introduction</a>
--------------

This ansible role is to mount devices to specified paths.

## <a name="2">Role Variables</a>
--------------

### Expected to Be Configured

* `mount_paths`:
  * `src_device`: specify the device to be mounted on `path`
  * `path`: specify the path to the mount point (e.g. `/mnt/files`)
  * `opts`: specify the mount options (such as `noatime`)
  * `fstype`: specify the filesystem type

### Proxy configuration options

### Role Defaults

## <a name="3">Example Playbook</a>
----------------

```
---
- hosts: mount_hosts 
  remote_user: estuaryapp
  become: yes
  vars:
    mount_paths:
      - src_device: "/dev/sdb"
        path: "/mount/path1"
        opts: ""
        fstype: ext4
      - src_device: "/dev/sdc"
        path: "/mount/path2"
        opts: "noatime"
        fstype: xfs
  roles:
    - mount

```    

For more examples, please refer to [e-commerce-springcloud-microservice](https://github.com/open-estuary/appbenchmark/tree/master/apps/e-commerce-solutions/e-commerce-springcloud-microservice)

License
-------

Apache

