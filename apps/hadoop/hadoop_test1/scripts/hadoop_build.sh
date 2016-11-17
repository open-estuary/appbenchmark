#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To install Hadoop
#
#####################################################################################
BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
VERSION="2.6.5"
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

#######################################################################################
if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/hadoop-dist/target/hadoop-${VERSION}.tar.gz)"  == 0 ]; then
      echo "Hadoop has been built successfully"
      exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
#rm -fr ${BUILD_DIR}
mkdir -p ${BUILD_DIR}
#tar -zxvf ${SERVER_FILENAME} -C ${BUILD_DIR}
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

if [ -z "$(grep MAVEN_OPTS /etc/profile)" ] ; then 
    echo 'export MAVEN_OPTS="-Xms512m -Xmx2000m"' >> /etc/profile
fi
echo "Finish build preparation......"

need_build=1
if [ ${need_build} -eq 0 ] ; then 
    echo "Not necessar to build Hadoop so far ....."
    exit 0
fi

######################################################################################
# Build Hadoop
######################################################################################
pushd ${BUILD_DIR}/${TARGET_DIR} > /dev/null
source /etc/profile
set Platform=aarch64

#Since it could not compile Hadoop by using jdk1.8, 
#so we will use jdk1.7 to compile Hadoop temporaily.
#However it will still use jdk1.8 to execute hadoop due to performance reason
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
mvn package -Pdist,native -DskipTests -Dtar 
JAVA_HOME=${OLD_JAVA_HOME}
popd > /dev/null

