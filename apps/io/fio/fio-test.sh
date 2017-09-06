#!/bin/sh

###################################################################################
# Global Variables
###################################################################################
CURPATH=`pwd`
FIOPATH=$CURPATH/fio

###################################################################################
# Usage
###################################################################################
Usage()
{
cat << EOF
Usage: ./sailing/build.sh [options]
Options:
	-h, --help: Display this information
	-v, --version: print fio scripts version
Options:
	--bs : block sector
	--rw : read/write style
	iodepth : the depth of io queue  
	--builddir: Build output directory, default is workspace
		
	
Example:
	./sailing/build.sh --help
	./sailing/build.sh --builddir=./workspace \\
		--deploy=pxe --mac=01-00-18-82-05-00-7f,01-00-18-82-05-00-68 \\
		--deploy=usb:/dev/sdb --deploy=iso	
	
EOF
}

###################################################################################
# COMPILE ENVIRONMENT CHECK
###################################################################################
envirocheck()
{
    if which fio ; then 
	#echo -e "\nfio has install    \n"
	return 1
    fi

    if rpm -qa | grep "libaio-dev"; then
            echo  -e "\n    libaio & libaio-dev has installed   \n"           
    else
            yum install -y libaio
            yum install -y libaio-devel
    fi
    return 0
}

###################################################################################
# DOWNLOAD TAG : fio-2.10 
###################################################################################

fioinstall()
{
    if [ ! -d "$CURPATH/fio"  ]; then
    	git clone https://github.com/hubzhangxj/fio.git -b fio-2.10
    else
    	echo  -e "\n    Fio has cloned, Note that version 2.10    \n"		
    fi
    pushd $FIOPATH
    ./configure
    make
    make install
    popd

    return 0
}

###################################################################################
# FIO TEST : fio-2.10                                                            
###################################################################################

fiotest()
{

echo "
[global]
rw=$rwstyle
direct=1
ramp_time=1
ioengine=libaio
iodepth=$iodepth
numjobs=1
bs=$blksectcor
#;size=102400m
;zero_buffers=1
group_reporting=1
;ioscheduler=noop
;gtod_reduce=1
;iodepth_batch=2
;iodepth_batch_complete=2
runtime=300
;thread
loops=10
" > ${blksector}_${rwstyle}_depth${iodepth}_fiotest
fiofile=${blksector}_${rwstyle}_depth${iodepth}_fiotest

declare -i new_count=1

if [ "$devtype"x = "nvme"x   ];then
    echo /dev/nvme0n1 > devinfo 
else
    fdisk -l | grep -o "/dev/[a-z]d[a-z][1-9]" > devinfo                                                      
fi

new_num=`sed -n '$=' devinfo`

while [ $new_count -le $new_num ]
do
	new_disk=`sed -n "$new_count"p devinfo`
	((new_count++))
	if [ "$new_disk" = "/dev/sd" ]; then
	continue
	fi
	echo "[job1]" >> $fiofile
	echo "filename=$new_disk" >> $fiofile

done

echo "Creat $fiofile file successfully"
    
chmod +x fio
fio $fiofile

}

###################################################################################
# Get all args
###################################################################################

while test $# != 0
do
        case $1 in
        	--*=*) ac_option=`expr "X$1" : 'X\([^=]*\)='` ; ac_optarg=`expr "X$1" : 'X[^=]*=\(.*\)'` ; ac_shift=: ;;
        	-*) ac_option=$1 ; ac_optarg=$2; ac_shift=shift ;;
        	*) ac_option=$1 ; ac_shift=: ;;
        esac

        case $ac_option in
                clean) CLEAN=yes ;;
                -h | --help) Usage ; exit 0 ;;      
                --bs) blksector=$ac_optarg ;;
                --rw) rwstyle=$ac_optarg ;;
                --iodepth) iodepth=$ac_optarg ;; 
                --devtype) devtype=$ac_optarg ;;
                *) Usage ; echo "Unknown option $1" ; exit 1 ;;
        esac
	
        $ac_shift
        shift
done

#if ! compile; then
#	echo -e "\033[31mError! Compile fio failed!\033[0m" ; exit 1
#fi

#reference: http://www.cnblogs.com/lichkingct/archive/2010/08/27/1810463.html
#ps -ef |grep fio |grep -v grep|cut -c 9-15|xargs kill -9thinktime=600
if ! envirocheck; then
	echo -e "\033[31mfio has installed!\033[0m"
else
    if ! fioinstall; then
	    echo -e "\033[31mError! Download & Install fio failed!\033[0m" 
    fi
fi

fiotest
