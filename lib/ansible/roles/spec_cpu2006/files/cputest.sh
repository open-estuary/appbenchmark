# !/bin/bash

CURR_WORK_PATH=$(cd `dirname $0`;pwd)

CURR_WORK_PATH=$CURR_WORK_PATH/speccpu2006
echo "path $CURR_WORK_PATH" >>/tmp/test12345
cd $CURR_WORK_PATH
echo "currpath = `pwd`" >>/tmp/test12345
source shrc
bin/relocate
source shrc

#Speed test begin
echo "Speed test..."
./bin/runspec -c lemon-2cpu.cfg all --speed -n 1 --noreportable

echo "Rate16 test..."
./bin/runspec -c lemon-2cpu.cfg all --rate 16 -n 1 --noreportable

echo "Rate32 testing"
./bin/runspec -c lemon-2cpu.cfg all --rate 32 -n 1 --noreportable

echo "Rate64 testing"
./bin/runspec -c lemon-2cpu.cfg all --rate 64 -n 1 --noreportable

