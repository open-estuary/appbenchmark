# !/bin/sh

REMOTE_SERVER_ADDR=192.168.1.107
REMOTE_SERVER_CPU_TEST_TOOL_PATH=/home/dingwei/InterPerf/speccpu2006
ANSIBLE_PATH=$(cd `dirname $0`; pwd)
CPU_TEST_TOOL_PKG=speccpu2006.tar.gz
TMP_OPT_DIR=/tmp

if [ $# -lt 1 ]; then
  echo "Usage ./get_cpu_test_tool.sh <BoardServer2UserName> <BoardServer2Ip,default for 192.168.1.107>"
  exit 1
fi

if [ $# -gt 2 ]; then
  echo "Usage ./get_cpu_test_tool.sh <BoardServer2UserName> <BoardServer2Ip,default for 192.168.1.107>"
  exit 1
fi

if [ $# -eq 2 ]; then
  echo "RemoteServerAddr ip is changed to $2"
  REMOTE_SERVER_ADDR=$2
fi

BoardServerUserName=$1

echo "check cputest tool in ansible path or not"
if [ -f $ANSIBLE_PATH/../../../lib/ansible/roles/spec_cpu2006/files/$CPU_TEST_TOOL_PKG ]; then
  echo "Cpu test tool pkg already in ansible path,jump out"
  exit 0
fi

echo "Starting pull cputest tool from remote server $REMOTE_SERVER_ADDR  ..."
scp -r $BoardServerUserName@$REMOTE_SERVER_ADDR:$REMOTE_SERVER_CPU_TEST_TOOL_PATH $TMP_OPT_DIR
if [ $? -ne 0 ]; then
  exit 1
fi

#echo "Starting compile cputest tool..."
#$ANSIBLE_PATH/compile_cpu_test_tool.sh $TMP_OPT_DIR/speccpu2006
#if [ $? -ne 0 ]; then
#  rm -rf $TMP_OPT_DIR/speccpu2006
#  exit 1
#fi

echo "Starting tar cputest tool"
cd $TMP_OPT_DIR
tar -czvf $TMP_OPT_DIR/$CPU_TEST_TOOL_PKG ./speccpu2006
cd -

echo "Starting copy cputest tar pktage to ansible dir"
mv $TMP_OPT_DIR/$CPU_TEST_TOOL_PKG $ANSIBLE_PATH/../../../lib/ansible/roles/spec_cpu2006/files
if [ $? -ne 0 ]; then
  rm -rf $TMP_OPT_DIR/speccpu2006
  rm -f $TMP_OPT_DIR/$CPU_TEST_TOOL_PKG
  exit 1
fi

echo "clear tmp files..."
rm -rf $TMP_OPT_DIR/speccpu2006
if [ $? -ne 0 ]; then
  exit 1
fi

echo "get cpu test tool and compile successfully"

