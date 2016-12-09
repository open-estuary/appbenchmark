#!/bin/bash

MYSQL="mysql -h ${1} -P3306 -p -uroot -t"

echo "Warming up..."

for i in $(seq 1 100)
do 
    ${MYSQL} sbtest >/dev/null <<EOF
select avg(id) from sbtest$i force key (primary);
select count(*) from sbtest$i WHERE k like '%0%';
EOF

done

