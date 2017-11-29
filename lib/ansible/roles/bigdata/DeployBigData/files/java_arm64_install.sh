# !/bin/bash

source /tmp/BigDataConfig

echo "start download java"
if [ ! -f jdk-${JAVA_DOWNLOAD_VERSION}-linux-arm64-vfp-hflt.tar.gz ] ; then
  wget -O jdk-${JAVA_DOWNLOAD_VERSION}-linux-arm64-vfp-hflt.tar.gz --no-check-certificate --no-cookies --header "Cookie: oraclelicense=accept-securebackup-cookie" http://download.oracle.com/otn-pub/java/jdk/${JAVA_DOWNLOAD_VERSION}-b12/e758a0de34e24606bca991d704f6dcbf/jdk-${JAVA_DOWNLOAD_VERSION}-linux-arm64-vfp-hflt.tar.gz
  if [ $? -ne 0 ]; then
    exit 1
  fi
fi

echo "decompress java packet"
tar -zxvf jdk-${JAVA_DOWNLOAD_VERSION}-linux-arm64-vfp-hflt.tar.gz -C $SOFTWARE_INSTALL_PATH
if [ $? -ne 0 ]; then
  exit 1
fi

#echo "clear tmp resource"
#rm -rf jdk-${JAVA_DOWNLOAD_VERSION}-linux-arm64-vfp-hflt.tar.gz*
#if [ $? -ne 0 ]; then
#  exit 1
#fi

