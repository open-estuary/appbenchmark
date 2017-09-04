# !/bin/sh

####if [ $# -ne 2 ]; then
####  echo "Usage: ./compile_speccpu2006.sh <DstWorkPath>" >>/tmp/test123
####  exit 0
####fi

####DstPath=$1

CURR_ROOT_DIR=$(cd `dirname $0`;pwd)

##### modify environment varible

echo "into compile script,Dstpath=$DstPath" >> /tmp/test123
export FORCE_UNSAFE_CONFIGURE=1
echo "after force unsafe configure" >> /tmp/test123
#### call buildtools script to finish speccpu2006 compile
####cd $DstPath/speccpu2006/tools/src
cd ${CURR_ROOT_DIR}/speccpu2006/tools/src
echo "curr dir is `pwd`" >> /tmp/test123
echo y | ./buildtools
echo "$?" >> /tmp/test123
cd -
