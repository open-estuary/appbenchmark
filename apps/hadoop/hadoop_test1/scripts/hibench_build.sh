#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build HiBench
#
#####################################################################################

BUILD_DIR="./build_hibench"
CTAG_VERSION="HiBench-5.0"
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/src/target/hibench-4.0-SNAPSHOT.jar)"  == 0 ]; then
      echo "Hibench has been built successfully"
      exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
#$(tool_add_sudo) rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR} > /dev/null

#To import JAVA_HOME
source /etc/profile

#Since it could not compile Hibench by using jdk1.8, 
#so we will use jdk1.7 to compile it temporaily.
#However it will still use jdk1.8 to execute Hibench due to performance reason
OLD_JAVA_HOME=${JAVA_HOME}
JAVA_1_7_HOME=""
for dirname in $(ls /usr/lib/jvm/) 
do
   if [ ! -d "/usr/lib/jvm/"${dirname} ] ; then
       continue
   fi
    
   if [[ "${dirname}" =~ ^"java-1.7.0-openjdk".* ]] ; then
       JAVA_1_7_HOME="/usr/lib/jvm/"${dirname}
       break
   fi
done

if [ -z "${JAVA_1_7_HOME}" ] ; then
   echo "Please install java-1.7.0-openjdk firstly"
   exit 0
fi

JAVA_HOME=${JAVA_1_7_HOME}

git clone https://github.com/intel-hadoop/HiBench.git
TARGET_DIR=$(tool_get_first_dirname ../${BUILD_DIR})
cd ./${TARGET_DIR}
git checkout ${CTAG_VERSION}

######################################################################################
# Build HiBench
#####################################################################################
./bin/build-all.sh
#mvn clean package
#mvn -Phadoopbench clean package
export JAVA_HOME=${OLD_JAVA_HOME}
popd > /dev/null

