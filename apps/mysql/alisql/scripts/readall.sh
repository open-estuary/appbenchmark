# Usage : readall <ip address> <user> <password> <table_num>
#

total=${4}
declare -i i=1

BASE_DIR=$(cd ~; pwd)

while ((i<=${total}))
do 
    echo ${i}
    sysbench --test=${BASE_DIR}/apptests/sysbench/tests/db/readall.lua \
             --oltp-test-mode=complex --oltp-read-only=on --mysql-host=${1} \
             --mysql-db=sysbench --mysql-password=${3}  \
             --max-requests=1 --mysql-user=${2} --mysql-table-engine=innodb \
             --oltp-table-size=1000000 \
             --oltp-tables-count=${i}  \
             --num-threads=1 --mysql-port=3306 run &
    let ++i
    sleep 10
done


