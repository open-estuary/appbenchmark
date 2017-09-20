* [Introduction](#1)
* [Benchmark Test](#2)

# FIO Benchmark Test 
## <a name="1">Introduction</a>
FIO is a tool for testing IOPS for stress testing and verification of hardware, supporting 13 different I/O engines, including: sync, mmap, libaio, posixaio, SG v3, splice, null, network, syslet , guasi, solarisaio and so on.

### FIO installation
###### 1. Source code
> wget http://brick.kernel.dk/snaps/fio-2.0.7.tar.gz

> yum install libaio-devel

> tar -zxvf fio-2.0.7.tar.gz

> cd fio-2.0.7

> make

> make install
###### 2. RPM
> yum install fio
### FIO usage
#### Random reading
> fio -filename=/dev/sdb1 -direct=1 -iodepth 1 -thread -rw=randread -ioengine=psync -bs=16k -size=200G -numjobs=10 -runtime=1000 -group_reporting -name=mytest

#### Description

```
filename = /dev/sdf    # test the file name, usually select the data directory of the disk to be tested.
direct = 1             # the test process bypasses the machine's own buffer. Make the test results more real.
rw = randwrite         # test randomly written I / O
rw = randrw            # test random write and read I / O
bs = 16k               # single io block file size is 16k
bsrange = 512-2048     # the size of the proposed data block range
size = 5g              # this test file size of 5g, to each 4k io test.
numjobs = 30           # this test thread is 30.
runtime = 1000         # test time is 1000 seconds, if not written will always be 5g file points 4k each time finished.
ioengine = psync       # io engine using pync mode
rwmixwrite = 30        # in the mixed read and write mode, write 30%
group_reporting        # for information about the results, summarize each process.
```


## <a name="2">Benchmark Test</a>

### Setup
#### Directory Structure


```
appbenchmark
      |-------[apps]
      |        |-------[io]
      |        |       |-------[fio]
      |        |       |        |-------ansible
      |        |       |        |-------setup.sh
      |        |       |        |-------run_all_test.sh
      |        |       |        |-------run_test.sh
      |        |       |-------sysbench
      |-------lib/ansible/roles
      |        |-------[io]
      |        |        |-------[fio]
      |        |        |       |-------files
      |        |        |       |-------vars
      |        |        |       |-------tasks
      |        |        |-------sysbench
      |-------toolset
```


###### 1. fio start script and test script
> [appbenchmark/apps/io/fio](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/fio)
- **[ansible](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/fio/ansible)** ( includeing roles and startup and test scripts )
- [run_all_test.sh](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/fio/run_all_test.sh) ( use the configuration file )
- [run_test.sh](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/fio/run_test.sh) （ do not use the configuration file ）
- [setup.sh](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/fio/setup.sh) ( create a directory structure on the target machine, copy files to the target machine, install fio )

###### note
- the target machine`s ip is stored in [hosts](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/fio/ansible/hosts)
- the test script is in the [files](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/fio/files) folder



###### 2. ansible roles

> [appbenchmark/lib/ansible/roles/io/fio](https://github.com/open-estuary/appbenchmark/tree/lib/ansible/roles/io/fio)

- [files](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/fio/files) ( test scripts )
- [tasks](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/fio/tasks) ( install and deploy scripts )
- [vars](https://github.com/open-estuary/appbenchmark/tree/master/lib/ansible/roles/io/fio/vars)  ( system variables )

#### setup

1. enter [appbenchmark/apps/io/fio/ansible/hosts](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/fio/ansible/hosts)
1. modify the target machine ip
1. enter [appbenchmark/apps/io/fio](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/fio)
1. run sh [setup.sh](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/fio/setup.sh)
### Test 

1. enter [appbenchmark/apps/io/fio](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/fio)
2. run   sh [run_test.sh](https://github.com/open-estuary/appbenchmark/tree/master/apps/io/fio/run_test.sh)

###### note
- After the test script starts running in the backgroud, you can view the process via [ps ux | grep ansible](#)
- The test results are stored in the [result_io](#) folder in the current user`s root directory. The result_io needs to be created by yourself.

### Test Results
                                           

```
read: (g=0): rw=read, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=512
...
write: (g=1): rw=write, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=512
...
randread: (g=2): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=512
...
randwrite: (g=3): rw=randwrite, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=512
...
randrw: (g=4): rw=randrw, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=512
...
fio-2.19
Starting 150 threads

read: (groupid=0, jobs=30): err= 0: pid=33895: Tue Sep 12 08:42:44 2017
   read: IOPS=119k, BW=463MiB/s (486MB/s)(45.4GiB/100203msec)
    slat (usec): min=3, max=202069, avg= 4.31, stdev=118.78
    clat (msec): min=7, max=600, avg=129.48, stdev=104.24
     lat (msec): min=7, max=600, avg=129.49, stdev=104.24
    clat percentiles (msec):
     |  1.00th=[   72],  5.00th=[   74], 10.00th=[   81], 20.00th=[   85],
     | 30.00th=[   87], 40.00th=[   89], 50.00th=[   95], 60.00th=[  113],
     | 70.00th=[  116], 80.00th=[  130], 90.00th=[  169], 95.00th=[  529],
     | 99.00th=[  562], 99.50th=[  562], 99.90th=[  570], 99.95th=[  578],
     | 99.99th=[  594]
    lat (msec) : 10=0.01%, 20=0.06%, 50=0.05%, 100=51.56%, 250=42.84%
    lat (msec) : 500=0.08%, 750=5.54%
  cpu          : usr=0.69%, sys=1.44%, ctx=470754, majf=0, minf=108595
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=0.1%, >=64=106.4%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.1%
     issued rwt: total=11874158,0,0, short=0,0,0, dropped=0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=512
write: (groupid=1, jobs=30): err= 0: pid=33967: Tue Sep 12 08:42:44 2017
  write: IOPS=113k, BW=441MiB/s (462MB/s)(43.2GiB/100148msec)
    slat (usec): min=2, max=45669, avg= 3.68, stdev=30.37
    clat (msec): min=8, max=5080, avg=136.58, stdev=213.11
     lat (msec): min=8, max=5080, avg=136.58, stdev=213.11
    clat percentiles (msec):
     |  1.00th=[   31],  5.00th=[  123], 10.00th=[  127], 20.00th=[  128],
     | 30.00th=[  128], 40.00th=[  129], 50.00th=[  129], 60.00th=[  129],
     | 70.00th=[  129], 80.00th=[  130], 90.00th=[  131], 95.00th=[  135],
     | 99.00th=[  159], 99.50th=[  167], 99.90th=[ 5014], 99.95th=[ 5080],
     | 99.99th=[ 5080]
    lat (msec) : 10=0.01%, 20=0.47%, 50=0.83%, 100=0.95%, 250=97.68%
    lat (msec) : 500=0.01%, >=2000=0.19%
  cpu          : usr=0.68%, sys=1.20%, ctx=479731, majf=0, minf=92998
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=0.1%, >=64=106.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.1%
     issued rwt: total=0,11289310,0, short=0,0,0, dropped=0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=512
randread: (groupid=2, jobs=30): err= 0: pid=34004: Tue Sep 12 08:42:44 2017
   read: IOPS=450, BW=1909KiB/s (1955kB/s)(190MiB/101797msec)
    slat (usec): min=3, max=2250.7k, avg=67270.00, stdev=372097.59
    clat (msec): min=25, max=34717, avg=28641.14, stdev=9691.14
     lat (msec): min=403, max=36845, avg=28707.65, stdev=9696.61
    clat percentiles (msec):
     |  1.00th=[ 1303],  5.00th=[ 5080], 10.00th=[10814], 20.00th=[16712],
     | 30.00th=[16712], 40.00th=[16712], 50.00th=[16712], 60.00th=[16712],
     | 70.00th=[16712], 80.00th=[16712], 90.00th=[16712], 95.00th=[16712],
     | 99.00th=[16712], 99.50th=[16712], 99.90th=[16712], 99.95th=[16712],
     | 99.99th=[16712]
    lat (msec) : 50=0.01%, 100=0.01%, 250=0.02%, 500=0.17%, 750=0.42%
    lat (msec) : 1000=0.07%, 2000=1.05%, >=2000=104.26%
  cpu          : usr=0.00%, sys=0.01%, ctx=1617, majf=0, minf=5557
  IO depths    : 1=0.1%, 2=0.1%, 4=0.3%, 8=0.5%, 16=1.0%, 32=2.1%, >=64=101.9%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=99.9%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.1%
     issued rwt: total=45827,0,0, short=0,0,0, dropped=0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=512
randwrite: (groupid=3, jobs=30): err= 0: pid=34037: Tue Sep 12 08:42:44 2017
  write: IOPS=369, BW=1563KiB/s (1600kB/s)(156MiB/102071msec)
    slat (usec): min=3, max=3476.3k, avg=82019.22, stdev=454589.06
    clat (msec): min=2, max=45416, avg=33512.88, stdev=12885.69
     lat (msec): min=507, max=45888, avg=33594.13, stdev=12893.04
    clat percentiles (msec):
     |  1.00th=[ 1369],  5.00th=[ 4817], 10.00th=[10028], 20.00th=[16712],
     | 30.00th=[16712], 40.00th=[16712], 50.00th=[16712], 60.00th=[16712],
     | 70.00th=[16712], 80.00th=[16712], 90.00th=[16712], 95.00th=[16712],
     | 99.00th=[16712], 99.50th=[16712], 99.90th=[16712], 99.95th=[16712],
     | 99.99th=[16712]
    lat (msec) : 4=0.01%, 50=0.01%, 100=0.01%, 250=0.02%, 500=0.03%
    lat (msec) : 750=0.43%, 1000=0.17%, 2000=1.78%, >=2000=103.15%
  cpu          : usr=0.00%, sys=0.00%, ctx=1342, majf=0, minf=4
  IO depths    : 1=0.1%, 2=0.2%, 4=0.3%, 8=0.6%, 16=1.3%, 32=2.5%, >=64=100.6%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=99.9%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.1%
     issued rwt: total=0,37760,0, short=0,0,0, dropped=0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=512
randrw: (groupid=4, jobs=30): err= 0: pid=34069: Tue Sep 12 08:42:44 2017
   read: IOPS=176, BW=753KiB/s (771kB/s)(74.1MiB/101904msec)
    slat (usec): min=3, max=3165.6k, avg=83113.51, stdev=467169.52
    clat (msec): min=96, max=44889, avg=34861.20, stdev=13641.29
     lat (msec): min=412, max=47576, avg=34942.45, stdev=13651.26
    clat percentiles (msec):
     |  1.00th=[ 1401],  5.00th=[ 5080], 10.00th=[10552], 20.00th=[16712],
     | 30.00th=[16712], 40.00th=[16712], 50.00th=[16712], 60.00th=[16712],
     | 70.00th=[16712], 80.00th=[16712], 90.00th=[16712], 95.00th=[16712],
     | 99.00th=[16712], 99.50th=[16712], 99.90th=[16712], 99.95th=[16712],
     | 99.99th=[16712]
  write: IOPS=177, BW=758KiB/s (776kB/s)(75.4MiB/101904msec)
    slat (usec): min=5, max=3152.6k, avg=88353.18, stdev=481760.14
    clat (msec): min=73, max=44889, avg=34740.16, stdev=13685.91
     lat (msec): min=412, max=47589, avg=34827.01, stdev=13689.78
    clat percentiles (msec):
     |  1.00th=[ 1319],  5.00th=[ 5080], 10.00th=[10421], 20.00th=[16712],
     | 30.00th=[16712], 40.00th=[16712], 50.00th=[16712], 60.00th=[16712],
     | 70.00th=[16712], 80.00th=[16712], 90.00th=[16712], 95.00th=[16712],
     | 99.00th=[16712], 99.50th=[16712], 99.90th=[16712], 99.95th=[16712],
     | 99.99th=[16712]
    lat (msec) : 100=0.01%, 250=0.02%, 500=0.13%, 750=0.27%, 1000=0.45%
    lat (msec) : 2000=0.98%, >=2000=104.78%
  cpu          : usr=0.00%, sys=0.01%, ctx=1327, majf=0, minf=10
  IO depths    : 1=0.1%, 2=0.2%, 4=0.3%, 8=0.7%, 16=1.3%, 32=2.7%, >=64=101.4%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=99.9%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.1%
     issued rwt: total=18007,18089,0, short=0,0,0, dropped=0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=512

Run status group 0 (all jobs):
   READ: bw=463MiB/s (486MB/s), 463MiB/s-463MiB/s (486MB/s-486MB/s), io=45.4GiB (48.7GB), run=100203-100203msec

Run status group 1 (all jobs):
  WRITE: bw=441MiB/s (462MB/s), 441MiB/s-441MiB/s (462MB/s-462MB/s), io=43.2GiB (46.4GB), run=100148-100148msec

Run status group 2 (all jobs):
   READ: bw=1909KiB/s (1955kB/s), 1909KiB/s-1909KiB/s (1955kB/s-1955kB/s), io=190MiB (199MB), run=101797-101797msec

Run status group 3 (all jobs):
  WRITE: bw=1563KiB/s (1600kB/s), 1563KiB/s-1563KiB/s (1600kB/s-1600kB/s), io=156MiB (163MB), run=102071-102071msec

Run status group 4 (all jobs):
   READ: bw=753KiB/s (771kB/s), 753KiB/s-753KiB/s (771kB/s-771kB/s), io=74.1MiB (78.7MB), run=101904-101904msec
  WRITE: bw=758KiB/s (776kB/s), 758KiB/s-758KiB/s (776kB/s-776kB/s), io=75.4MiB (79.6MB), run=101904-101904msec

Disk stats (read/write):
  sdf: ios=167636/156760, merge=12354486/11696659, ticks=32177111/40492902, in_queue=72794219, util=100.00%
```
