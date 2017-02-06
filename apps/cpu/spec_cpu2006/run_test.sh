#!/bin/bash

#Basic commands
#./shrc

SPECROOT=${CPUSPEC_DIR}
#Speed test
echo "Speed test ..."
runspec -c ${SPECROOT}/config/lemon-2cpu.cfg all --speed -n 1 --noreportable

echo "Rate16 test..."
runspec -c ${SPECROOT}/config/lemon-2cpu.cfg all --rate 16 -n 1 --noreportable

echo "Rate32 test..."
runspec -c ${SPECROOT}/config/lemon-2cpu.cfg all --rate 32 -n 1 --noreportable

echo "Rate64 test..."
runspec -c ${SPECROOT}/config/lemon-2cpu.cfg all --rate 64 -n 1 --noreportable


#Run specific test case 
#testcaseno=1
#runspec -c ${SPECROOT}/config/lemon-2cpu.cfg ${testcaseno} --rate 64 -n 1 --noreportable
