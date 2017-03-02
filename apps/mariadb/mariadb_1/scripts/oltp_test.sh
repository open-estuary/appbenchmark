#!/bin/bash
#
# Run SysBench tests with MariaDB and MySQL
#
IP=${1}
USER=${2}
PASSWORD=${3}

SYSBENCH_TEST=${4}
if [ -z "${4}" ] ; then
    echo "Must specify one test case name"
    exit 1
fi

THREADS=${5}
if [ -z "${5}" ] ; then
    echo "Must specify the number of threads"
    exit 1
fi

BASE="$(cd ~; pwd)""/apptests/sysbench"
TEST_DIR="${BASE}/tests/db"
TEMP_DIR="${BASE}/tmp"
SYSBENCH="/usr/local/bin/sysbench"

# The table size we use for SysBench.
TABLE_SIZE=2000000

# The run time we use for SysBench.
RUN_TIME=3600

# Warm up time we use for SysBench.
WARM_UP_TIME=300

# How many times we run each test.
LOOP_COUNT=1

# We need at least 1 GB disk space in our $WORK_DIR.
SPACE_LIMIT=1000000

# Interval in seconds for monitoring system status like disk IO,
# CPU utilization, and such.
MONITOR_INTERVAL=10

SYSBENCH_OPTIONS="--oltp-table-size=$TABLE_SIZE \
  --max-requests=0 \
  --mysql-table-engine=InnoDB \
  --mysql-user=${USER} \
  --mysql-host=${IP} \
  --mysql-password=${PASSWORD}
  --mysql-engine-trx=yes \
  --rand-seed=303 \
  --db-driver=mysql"

# Timeout in seconds for waiting for mysqld to start.
TIMEOUT=100

#
# Directories.
# ${BASE} and ${TEMP_DIR} are defined in the $HOSTNAME.inc configuration file.
#
RESULT_DIR="${BASE}/sysbench-results"
SYSBENCH_DB_BACKUP="${TEMP_DIR}/sysbench_db"

if [ ! -d ${RESULT_DIR} ] ; then
    mkdir -p ${RESULT_DIR}
fi

if [ ! -d ${SYSBENCH_DB_BACKUP} ] ; then
    mkdir -p ${SYSBENCH_DB_BACKUP}
fi

#
# Write out configurations used for future reference.
#
#echo $MYSQLD_OPTIONS > ${RESULT_DIR}/${TODAY}/${PRODUCT}/mysqld_options.txt
echo $SYSBENCH_OPTIONS > ${RESULT_DIR}/${TODAY}/${PRODUCT}/sysbench_options.txt
echo '' >> ${RESULT_DIR}/${TODAY}/${PRODUCT}/sysbench_options.txt
echo "Warm up time is: $WARM_UP_TIME" >> ${RESULT_DIR}/${TODAY}/${PRODUCT}/sysbench_options.txt
echo "Run time is: $RUN_TIME" >> ${RESULT_DIR}/${TODAY}/${PRODUCT}/sysbench_options.txt

# If we run the same SysBench test with different options,
# then we have to take care to not overwrite our previous results.
m=0
DIR_CREATED=-1
MKDIR_RETRY=512
DIR_TO_CREATE="${RESULT_DIR}/${TODAY}/${PRODUCT}/${SYSBENCH_TEST}"

if [ ! -d $DIR_TO_CREATE ]; then
    mkdir $DIR_TO_CREATE
    CURRENT_RESULT_DIR="$DIR_TO_CREATE"
else 
    while [ $m -le $MKDIR_RETRY ]
        do
        if [ ! -d ${DIR_TO_CREATE}-${m} ]; then
            mkdir ${DIR_TO_CREATE}-${m}
            CURRENT_RESULT_DIR="${DIR_TO_CREATE}-${m}"
            DIR_CREATED=1

            break
        fi

        m=$(($m + 1))
    done

    if [ $DIR_CREATED = -1 ]; then
        echo "[ERROR]: Could not create result dir after $MKDIR_RETRY times."
        echo '  Please check your configuration and file system.'
        echo '  Refusing to overwrite existing results. Exiting!'

        exit 1
    fi
fi

echo "[$(date "+%Y-%m-%d %H:%M:%S")] Preparing and loading data for ${SYSBENCH_TEST}."
SYSBENCH_OPTIONS="${SYSBENCH_OPTIONS} ${TEST_DIR}/${SYSBENCH_TEST}"
$SYSBENCH $SYSBENCH_OPTIONS --time=$RUN_TIME prepare

THIS_RESULT_DIR="${CURRENT_RESULT_DIR}/${THREADS}"
    mkdir $THIS_RESULT_DIR
    echo "[$(date "+%Y-%m-%d %H:%M:%S")] Running ${SYSBENCH_TEST} with $THREADS threads and $LOOP_COUNT iterations for $PRODUCT" | tee ${THIS_RESULT_DIR}/results.txt
    echo '' >> ${THIS_RESULT_DIR}/results.txt

    SYSBENCH_OPTIONS_WARM_UP="${SYSBENCH_OPTIONS} --threads=3 --time=$WARM_UP_TIME"
    SYSBENCH_OPTIONS_RUN="${SYSBENCH_OPTIONS} --threads=$THREADS --time=$RUN_TIME"
    
    echo 3 | $SUDO tee /proc/sys/vm/drop_caches

    echo "[$(date "+%Y-%m-%d %H:%M:%S")] Starting warm up of $WARM_UP_TIME seconds."
    $SYSBENCH $SYSBENCH_OPTIONS_WARM_UP run
    sync
    echo "[$(date "+%Y-%m-%d %H:%M:%S")] Finnished warm up."
    
    k=0
    while [ $k -lt $LOOP_COUNT ] 
    do
        echo "[$(date "+%Y-%m-%d %H:%M:%S")] Starting actual SysBench run(${k})."

        $SYSBENCH $SYSBENCH_OPTIONS_RUN run > ${THIS_RESULT_DIR}/result${k}.txt 2>&1

        sync
        sleep 1

        grep "write requests:" ${THIS_RESULT_DIR}/result${k}.txt | awk '{ print $4 }' | sed -e 's/(//' >> ${THIS_RESULT_DIR}/results.txt
        k=$(($k + 1))
    done
    echo '' >> ${THIS_RESULT_DIR}/results.txt
    echo "[$(date "+%Y-%m-%d %H:%M:%S")] Finnished ${SYSBENCH_TEST} with $THREADS threads and $LOOP_COUNT iterations for $PRODUCT" | tee -a ${THIS_RESULT_DIR}/results.txt

#
# We are done!
#

#Delete test datas
/usr/local/mariadb/bin/mysql -h ${IP} -u ${USER} -p${PASSWORD} -P3306 << EOF
drop database sbtest;
create database sbtest;
EOF

echo "[$(date "+%Y-%m-%d %H:%M:%S")] Finished SysBench runs."
echo "  You can check your results."
