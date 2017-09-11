* [Introduction](#1)
* [Benchmark Test Scripts Guidelines](#2)
* [Others](#3)

## <a name="1">Introduction</a>
This document will describe how to write new test cases for each application.

## <a name="2">Benchmark Test Scripts Guidelines</a>
Let's suppose that we want to write test case for `packageA`

### File Strucutes Overview ###
The new test case should be based on [ansible](http://docs.ansible.com) which use `role` to reuse scripts. Therefore it is suggested to reuse [existing roles](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles) as much as possible.

Usually the following files and directories need to be prepared for `packageA`:
```
apps
├── packageA
│   └── packageA_test1
│       ├── ansible
│       │   └── roles
|       |        └── mysql -> ../../../../../lib/ansible/roles/mysql (reuse existing role)
|       |        ├── new role
|       |   ├── hosts ( which specify test client/servers) 
|       |   ├── site.yml (which specify what need to performed)
|       |   ├── group_vars
|       |        └── all (which define global variables)
│       ├── run_test.sh
│       └── setup.sh

```
### Test Setup Scripts
The `setup.sh` will calls the corresponding `ansible scripts` to setup server and client sides simultaneously.

### Test Exection Scripts
Usually the `run_test.sh` will be executed to run benchmark test case on client side. 

## <a name="3">Others</a>
