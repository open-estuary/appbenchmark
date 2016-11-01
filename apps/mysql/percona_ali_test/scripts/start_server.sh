#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To start Percona-Server 
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})
SUBBUILD_DIR="armbuild"

if [ "$(ps -aux | grep "/u01/my3306/bin/mysqld_safe" | grep -v "grep")" != "" ]; then
    echo "Percona server is running"
    exit 0
fi

#######################################################################################
check_startup_str() {
    expected_str=${1}
    max_retry_num=20
    cur_retry=0
    has_started=0
    while [[ ${cur_retry} -lt ${max_retry_num} ]] ;
    do
        echo "Check whether server has started yet or not ......"
        if [ $(tool_check_exists "/u01/my3306/log/alert.log") == 0 ] ; then 
            CHECK_STR=$($(tool_add_sudo) grep "${expected_str}" /u01/my3306/log/alert.log)
            if [ "${CHECK_STR}" ] ; then
                let "has_started=1"
                echo "Percona Server has started successfully"
                break
            fi
        fi
        let "cur_retry++"
        sleep 5
    done

    if [ ${has_started} -eq 0 ]; then
        echo "Hmm...Please check alert.log manually to see why the server has not started yet"
    fi
}

###########################################################################################
# Begin to start percona server
###########################################################################################

#Run step 1: Add 'mysql' test user account and rights
$(tool_add_sudo) groupadd mysql
$(tool_add_sudo) useradd -g mysql mysql

#Run step 2: Prepare for configuration for mysql
#Backup existing conf if necessary
if [ $(tool_check_exists "/etc/my.conf") == 0 ]; then
    cur_day_str=`date +%Y-%m-%d`
    echo "Backup existing /etc/my.conf ......."
    if [ $(tool_check_exists "/etc/my.conf_${cur_day_str}") != 0 ]; then
        $(tool_add_sudo) cp /etc/my.conf /etc/my.conf_${cur_day_str}
    fi
fi

$(tool_add_sudo) cp -f ${APP_ROOT}/apps/mysql/percona_ali_test/config/my.conf /etc/my.conf

$(tool_add_sudo) mkdir -p /u01/mysql
$(tool_add_sudo) cp -fr /u01/my3306/share /u01/mysql
$(tool_add_sudo) mkdir -p /u01/my3306/tmp
$(tool_add_sudo) mkdir -p /u01/my3306/log
$(tool_add_sudo) mkdir -p /u01/my3306/run

cur_user=`whoami`
$(tool_add_sudo) chown -L -R mysql.${cur_user} /u01

#Run step 3: Initialize database
INSTALL_DB_CMD="/u01/my3306/scripts/mysql_install_db --defaults-file=/etc/my.conf "
new_start_flag=0
if [ $(tool_check_exists "/u01/my3306/scripts/mysql_install_db") != 0 ] ; then
    new_start_flag=1
    INSTALL_DB_CMD="/u01/my3306/bin/mysqld  --defaults-file=/etc/my.conf --initialize"
fi

echo "Start to initialize database ......"
$(tool_add_sudo) ${INSTALL_DB_CMD}   --basedir=/u01/my3306 \
                                     --datadir=/u01/my3306/data \
                                     --user=mysql

#Run step 4: Start mysql server
echo "Start to run mysql server ......"
if [ ${new_start_flag} -eq 1 ] ; then 
############ Start Server by using new ways for 5.7+ versions ######################
$(tool_add_sudo) /u01/my3306/bin/mysqld_safe  --defaults-file=/etc/my.conf \
                                              --user=mysql \
                                              --skip-grant-tables \
                                              --skip-networking  \
                                              --basedir=/u01/my3306 \
                                              --datadir=/u01/my3306/data &

check_startup_str "Starting mysqld daemon with databases"
rm -f /u01/my3306/log/alert.log
sleep 30
/u01/my3306/bin/mysql -uroot << EOF
UPDATE mysql.user SET authentication_string=PASSWORD('123456') WHERE user='root';
UPDATE mysql.user SET authentication_string=PASSWORD('123456') WHERE user='mysql';
GRANT ALL PRIVILEGES ON *.* TO mysql@localhost IDENTIFIED BY '123456' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON *.* TO mysql@"%" IDENTIFIED BY '123456' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON *.* TO root@localhost IDENTIFIED BY '123456' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON *.* TO root@"%" IDENTIFIED BY '123456' WITH GRANT OPTION;
create database sysbench;
flush privileges;
exit;
EOF

#Restart server again 
ps -aux | grep mysql | grep -v grep | grep -v start | awk '{print $2}' | xargs kill -9
$(tool_add_sudo) /u01/my3306/bin/mysqld_safe --defaults-file=/etc/my.conf  \
                            --basedir=/u01/my3306 \
                            --datadir=/u01/my3306/data &

check_startup_str "Starting mysqld daemon with databases"
sleep 30
/u01/my3306/bin/mysql -uroot -p'123456'<< EOF
ALTER USER USER() IDENTIFIED BY '123456';
create database sysbench;
exit;
EOF

else 
########### Start server by using old ways for pre 5.7 versions ####################
$(tool_add_sudo) /u01/my3306/bin/mysqld_safe --defaults-file=/etc/my.conf  \
                            --basedir=/u01/my3306 \
                            --datadir=/u01/my3306/data &

#Run step 5: Check whether server has started successfully or not
check_startup_str "ready for connection"

#Install Step 6:set root rights and create initial database
/u01/my3306/bin/mysql -uroot << EOF
SET PASSWORD=PASSWORD('123456');
UPDATE mysql.user SET password=PASSWORD('123456') WHERE user='mysql';
GRANT ALL PRIVILEGES ON *.* TO mysql@localhost IDENTIFIED BY '123456' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON *.* TO mysql@"%" IDENTIFIED BY '123456' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON *.* TO root@localhost IDENTIFIED BY '123456' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON *.* TO root@"%" IDENTIFIED BY '123456' WITH GRANT OPTION;
create database sysbench;
EOF

fi

echo "Pecora server build and install complete"
