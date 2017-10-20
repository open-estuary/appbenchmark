#!/bin/env bash

fileM=$1
nfEnd=$2
if [ "X$fileM" == "X" ] ; then
fileM="1.txt"
fi
if [ "X$nfEnd" == "X" ] ; then
nfEnd="bin"
fi
#
rm -rf f_*.$nfEnd
rm -rf 1_1.$nfEnd
rm -rf dataGen.$nfEnd
#
#oneLine=`cat 1.txt`
#oneLine=$oneLine`echo "\n"`
fileNum=50 #gen count
#1M
echo `date`
lineCnt=""
for((j=1;j<1000;j++))
do
#lineCnt="$lineCnt$oneLine"
    cat $fileM >> 1_1.$nfEnd
done
#echo $lineCnt>>1_1.bin
echo `date`
#
temp_fifo_file=$$.info        #  管道
mkfifo $temp_fifo_file        #
exec 6<>$temp_fifo_file       # 管道命名6
rm $temp_fifo_file            #清空内容
  
function eachLineGen                
{  
  #sleep 2
}

temp_thread=50               #  同时最多线程数
  
for ((i=0;i<temp_thread;i++)) #  管道占位
do  
  echo                        #  
done >&6                      # 
  
for ((i=0;i<$fileNum;i++))    #需要运行的总数线程        
do  
  read                        #  
  {  
    #f_sleep  
    echo $$,$i,`date`
    #lineCntNew=$lineCnt
    for((j=1;j<20;j++))
    do
        #lineCntNew="$lineCntNew$lineCnt"
        cat 1_1.$nfEnd>>f_$i.$nfEnd
    done
    #  
    #echo $lineCntNew >>f_$i.bin  
    echo $$,$i,`date`  
    echo >&6                  #  完成一个，补充管道占位
  }&                          #  能获取到管道中内容，继续起线程                     
done <&6  
wait                          #
exec 6>&-                     #完成，删除管道

echo `date`
for ((i=0;i<$fileNum;i++))
do
cat f_$i.$nfEnd >>dataGen.$nfEnd
rm -rf f_$i.$nfEnd
done

echo "done!"

