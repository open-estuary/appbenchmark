
ip=${1}
user=${2}
password=${3}
read_only=${4}
active=${5}
unactive=${6}
ip=${7}
db=${8}
tables=${9}
size=${10}
scripts=${11}

declare -i i=1

BASE_DIR=$(cd ~; pwd)

while ((i<=${active}))
do 
    echo ${i}
    sysbench --test=${BASE_DIR}/apptests/sysbench/tests/db/${scripts}.lua \
             --tx-rate=100 --oltp-test-mode=complex --oltp-read-only=${read_only} \
             --mysql-host=${ip} --mysql-db=sysbench --mysql-password=${password} \
             --max-requests=0 --mysql-user=${user} \
             --mysql-table-engine=innodb \
             --oltp-table-size=${size} \
             --oltp-tables-count=${tables} \
             --num-threads=10 --mysql-port=3306 run &
    let ++i
    sleep ${12}
done

i=1
while ((i<=${unactive}))
do 
    echo ${i}
    sysbench --test=${BASE_DIR}/apptests/sysbench/tests/db/${scripts}.lua \
             --tx-rate=100 --oltp-test-mode=complex --oltp-read-only=${read_only} \
             --mysql-host=${ip} --mysql-db=sysbench --mysql-password=${password}  \
             --max-requests=0 --mysql-user=root --mysql-table-engine=innodb --oltp-table-size=${size}  \
             --oltp-tables-count=${tables}  --num-threads=10 --mysql-port=3306 run &
    let ++i
    sleep  ${12}
done

