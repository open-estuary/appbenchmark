#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To start MariaDB-Server 
#
#####################################################################################

cmd_str="${1}"

if [ "$(ps -aux | grep "/usr/local/mariadb/bin/mysqld_safe" | grep -v "grep")" != "" ]; then
    echo "MariaDB server is running"
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
        if [ $(tool_check_exists "/u01/mariadb/log/alert.log") == 0 ] ; then 
            CHECK_STR=$($(tool_add_sudo) grep "${expected_str}" /u01/mariadb/log/alert.log)
            if [ "${CHECK_STR}" ] ; then
                let "has_started=1"
                echo "MariaDB Server has started successfully"
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
# Begin to start mariadb server
###########################################################################################
initialize_mysql_inst() {
    config_file=${1}
    
    #Run step 1: Add 'mysql' test user account and rights

    if id -u "mysql" > /dev/null 2>&1; then
        echo "Mysql user exist" > /dev/null
    else 
        $(tool_add_sudo) groupadd mysql
        $(tool_add_sudo) useradd -g mysql mysql
    fi
    
    #Backup existing conf if necessary
    if [ $(tool_check_exists "/etc/my_mariadb.conf") == 0 ]; then
        cur_day_str=`date +%Y-%m-%d`
        echo "Backup existing /etc/my_mariadb.conf ......"
        if [ $(tool_check_exists "/etc/my_mariadb.conf_${cur_day_str}") != 0 ]; then
            $(tool_add_sudo) cp /etc/my_mariadb.conf /etc/my_mariadb.conf_${cur_day_str}
        fi
    fi

    $(tool_add_sudo) cp -f ${APP_ROOT}/apps/mariadb/mariadb_1/config/${config_file} /etc/my_mariadb.conf
    new_port=3306
    sed -i "s/port=3306/port=${new_port}/g" /etc/my_mariadb.conf

    $(tool_add_sudo) echo 6553600 > /proc/sys/fs/aio-max-nr
    $(tool_add_sudo) echo 1 > /proc/sys/net/ipv4/tcp_timestamps
    $(tool_add_sudo) echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse
    $(tool_add_sudo) echo 1 > /proc/sys/net/ipv4/tcp_tw_recycle
    $(tool_add_sudo) echo 2048 65000 > /proc/sys/net/ipv4/ip_local_port_range
    $(tool_add_sudo) echo 2621440 > /proc/sys/net/core/somaxconn
    $(tool_add_sudo) echo 2621440 > /proc/sys/net/core/netdev_max_backlog
    $(tool_add_sudo) echo 2621440 > /proc/sys/net/ipv4/tcp_max_syn_backlog
    $(tool_add_sudo) echo 1 > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_time_wait
    $(tool_add_sudo) echo 2621440 > /proc/sys/net/netfilter/nf_conntrack_max
    $(tool_add_sudo) echo 10 > /proc/sys/net/ipv4/tcp_fin_timeout

    cur_user=`whoami`
    $(tool_add_sudo) mkdir -p /u01/mariadb/tmp
    $(tool_add_sudo) mkdir -p /u01/mariadb/log
    $(tool_add_sudo) mkdir -p /u01/mariadb/run
    $(tool_add_sudo) chown -L -R mysql.${cur_user} /u01/mariadb
    
    #Initialize database
    INSTALL_DB_CMD="/usr/local/mariadb/bin/mysql_install_db --defaults-file=/etc/my_mariadb.conf "
    new_start_flag=0
    #if [ $(tool_check_exists "/usr/local/mariadb/bin/mysql_install_db") != 0 ] ; then
    #    new_start_flag=1
    #    INSTALL_DB_CMD="/usr/local/mariadb/bin/mysqld  --defaults-file=/etc/my_mariadb.conf --initialize"
    #fi

    echo "Start to initialize database ......"
    $(tool_add_sudo) ${INSTALL_DB_CMD}   --basedir=/usr/local/mariadb \
                                     --datadir=/u01/mariadb/data \
                                     --user=mysql

    #Start mysql server
    echo "Initialize servers successfully"
}

######################################################################################
start_mysql_inst() {
    config_file=${1}
    
    new_start_flag=0
    if [ $(tool_check_exists "/usr/local/mariadb/bin/mysql_install_db") != 0 ] ; then
        new_start_flag=1
    fi
        
    rm -fr /u01/mariadb/log/alert.log
    echo "Restart server ......"
    ########### Start server by using old ways for pre 5.7 versions ####################
        $(tool_add_sudo) /usr/local/mariadb/bin/mysqld_safe --defaults-file=/etc/my_mariadb.conf  \
                            --basedir=/usr/local/mariadb \
                            --datadir=/u01/mariadb/data &

        #Check whether server has started successfully or not
        check_startup_str "ready for connection"

        #Install Step 6:set root rights and create initial database
        /usr/local/mariadb/bin/mysql -uroot --socket=/u01/mariadb/run/mysql.sock << EOF
SET PASSWORD=PASSWORD('123456');
UPDATE mysql.user SET password=PASSWORD('123456') WHERE user='mysql';
GRANT ALL PRIVILEGES ON *.* TO mysql@localhost IDENTIFIED BY '123456' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON *.* TO mysql@"%" IDENTIFIED BY '123456' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON *.* TO root@localhost IDENTIFIED BY '123456' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON *.* TO root@"%" IDENTIFIED BY '123456' WITH GRANT OPTION;
create database sysbench;
create database sbtest;
EOF
}


if [ "x${cmd_str}" == "xinit" ] ; then
    initialize_mysql_inst "my_mariadb.conf"
elif [ "x${cmd_str}" == "xstart" ] || [ "x${cmd_str}" == "xrestart" ] ; then
    start_mysql_inst "my_mariadb.conf"
else
    echo "Unknown commands:${cmd_str}"
fi
echo "========================================================================================="
