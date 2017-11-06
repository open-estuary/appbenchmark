#!/bin/bash


CURDIR=$(cd `dirname $0`; pwd)

pushd ${CURDIR}/ansible > /dev/null

ansible-playbook -i hosts site.yml --user=root --extra-vars "ansible_sudo_pass=root"  



########begin configure redis server###########
#if [ -z "${APP_ROOT}" ]; then
    # Default value
#    APP_ROOT=$(cd `dirname $0` ; cd ../../../; pwd)
#else
    # Re-declare so it can be used in this script
#    APP_ROOT=$(echo $APP_ROOT)
#fi


#${APP_ROOT}/redis/redis_test1/scripts/configure_redis_server.sh

popd > /dev/null

