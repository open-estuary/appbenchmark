#!/bin/bash


/u01/my3306/bin/mysql -uroot --connect-expired-password --socket=/u0/my3306/run/mysql.sock -p'123456'<< EOF
ALTER USER USER() IDENTIFIED BY '123456';
GRANT ALL PRIVILEGES ON *.* TO mysql@localhost IDENTIFIED BY '123456' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON *.* TO mysql@"%" IDENTIFIED BY '123456' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON *.* TO root@localhost IDENTIFIED BY '123456' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON *.* TO root@"%" IDENTIFIED BY '123456' WITH GRANT OPTION;
flush privileges;
create database sysbench;
exit
EOF

