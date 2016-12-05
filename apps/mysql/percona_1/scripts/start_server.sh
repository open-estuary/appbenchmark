#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To start Percona-Server 
#
#####################################################################################

cmd_str="${1}"

if [ "$(ps -aux | grep "/u01/my3306/bin/mysqld_safe" | grep -v "grep")" != "" ]; then
    echo "Percona server is running"
    exit 0
fi

#######################################################################################
check_startup_str() {
    local inst_num=${1}
    expected_str=${2}
    max_retry_num=20
    cur_retry=0
    has_started=0
    while [[ ${cur_retry} -lt ${max_retry_num} ]] ;
    do
        echo "Check whether server-${inst_num} has started yet or not ......"
        if [ $(tool_check_exists "/u01/u${inst_num}/my3306/log/alert.log") == 0 ] ; then 
            CHECK_STR=$($(tool_add_sudo) grep "${expected_str}" /u01/u${inst_num}/my3306/log/alert.log)
            if [ "${CHECK_STR}" ] ; then
                let "has_started=1"
                echo "Percona Server-${inst_num} has started successfully"
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
initialize_mysql_inst() {
    local inst_num=${1}  
    config_file=${2}
    
    #Run step 1: Add 'mysql' test user account and rights

    if id -u "mysql" > /dev/null 2>&1; then
        echo "Mysql user exist" > /dev/null
    else 
        $(tool_add_sudo) groupadd mysql
        $(tool_add_sudo) useradd -g mysql mysql
    fi

    #Backup existing conf if necessary
    if [ $(tool_check_exists "/etc/my_${inst_num}.conf") == 0 ]; then
        cur_day_str=`date +%Y-%m-%d`
        echo "Backup existing /etc/my_${inst_num}.conf ......"
        if [ $(tool_check_exists "/etc/my_${inst_num}.conf_${cur_day_str}") != 0 ]; then
            $(tool_add_sudo) cp /etc/my_${inst_num}.conf /etc/my_${inst_num}.conf_${cur_day_str}
        fi
    fi

    $(tool_add_sudo) cp -f ${APP_ROOT}/apps/mysql/percona_ali_1/config/${config_file} /etc/my_${inst_num}.conf
    sed -i "s/u01/u01\/u${inst_num}/g" /etc/my_${inst_num}.conf
    new_port=3306
    let "new_port+=${inst_num}"
    sed -i "s/port=3306/port=${new_port}/g" /etc/my_${inst_num}.conf

    $(tool_add_sudo) mkdir -p /u01/u${inst_num}/mysql
    $(tool_add_sudo) cp -fr /u01/my3306/share /u01/u${inst_num}/mysql
    $(tool_add_sudo) mkdir -p /u01/u${inst_num}/my3306/tmp
    $(tool_add_sudo) mkdir -p /u01/u${inst_num}/my3306/log
    $(tool_add_sudo) mkdir -p /u01/u${inst_num}/my3306/run

    cur_user=`whoami`
    $(tool_add_sudo) chown -L -R mysql.${cur_user} /u01/u${inst_num}
    $(tool_add_sudo) chown -L -R mysql.${cur_user} /u01
    
    #Initialize database
    INSTALL_DB_CMD="/u01/my3306/scripts/mysql_install_db --defaults-file=/etc/my_${inst_num}.conf "
    new_start_flag=0
    if [ $(tool_check_exists "/u01/my3306/scripts/mysql_install_db") != 0 ] ; then
        new_start_flag=1
        INSTALL_DB_CMD="/u01/my3306/bin/mysqld  --defaults-file=/etc/my_${inst_num}.conf --initialize"
    fi

    echo "Start to initialize database-${inst_num} ......"
    $(tool_add_sudo) ${INSTALL_DB_CMD}   --basedir=/u01/u${inst_num}/my3306 \
                                     --datadir=/u01/u${inst_num}/my3306/data \
                                     --user=mysql

    #Start mysql server
    echo "Initialize servers-${inst_num} successfully"
}

######################################################################################
start_mysql_inst() {

    local inst_num=${1}  
    config_file=${2}
    
    new_start_flag=0
    if [ $(tool_check_exists "/u01/my3306/scripts/mysql_install_db") != 0 ] ; then
        new_start_flag=1
    fi

    echo "Restart server-${inst_num}......"
    ########### Start server by using old ways for pre 5.7 versions ####################
        $(tool_add_sudo) /u01/my3306/bin/mysqld_safe --defaults-file=/etc/my_${inst_num}.conf  \
                            --basedir=/u01/u${inst_num}/my3306 \
                            --datadir=/u01/u${inst_num}/my3306/data &

        #Check whether server has started successfully or not
        check_startup_str ${inst_num} "ready for connection"

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
}


if [ "x${cmd_str}" == "xinit" ] then
    initialize_mysql_inst "00" "my.conf"
elif [ "x${cmd_str}" == "xstart" ] || [ "x${cmd_str}" == "xrestart" ] ; then
    start_mysql_inst "00" "my.conf"
else
    echo "Unknown commands:${cmd_str}"
fi
echo "========================================================================================="
