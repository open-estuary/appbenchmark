#!/bin/env bash

fileM=$1
nfEnd=$2if [ "X$fileM" == "X" ] ; thenfileM="1.txt"fiif [ "X$nfEnd" == "X" ] ; thennfEnd="bin"fi#rm -rf f_*.$nfEndrm -rf 1_1.$nfEndrm -rf dataGen.$nfEnd##oneLine=`cat 1.txt`#oneLine=$oneLine`echo "\n"`fileNum=50 #gen count#1Mecho `date`lineCnt=""for((j=1;j<1000;j++))do#lineCnt="$lineCnt$oneLine"    cat $fileM >> 1_1.$nfEnddone#echo $lineCnt>>1_1.binecho `date`#temp_fifo_file=$$.info        #  管道mkfifo $temp_fifo_file        #exec 6<>$temp_fifo_file       # 管道命名6rm $temp_fifo_file            #清空内容  function eachLineGen                {    #sleep 2}
temp_thread=50               #  同时最多线程数  for ((i=0;i<temp_thread;i++)) #  管道占位do    echo                        #  done >&6                      #   for ((i=0;i<$fileNum;i++))    #需要运行的总数线程        do    read                        #    {      #f_sleep      echo $$,$i,`date`    #lineCntNew=$lineCnt    for((j=1;j<20;j++))    do        #lineCntNew="$lineCntNew$lineCnt"        cat 1_1.$nfEnd>>f_$i.$nfEnd    done    #      #echo $lineCntNew >>f_$i.bin      echo $$,$i,`date`      echo >&6                  #  完成一个，补充管道占位  }&                          #  能获取到管道中内容，继续起线程                     done <&6  wait                          #exec 6>&-                     #完成，删除管道
echo `date`for ((i=0;i<$fileNum;i++))docat f_$i.$nfEnd >>dataGen.$nfEndrm -rf f_$i.$nfEnddone
echo "done!"
