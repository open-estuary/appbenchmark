### 本文介绍的数据生成工具的是在开源数据生成器[Data-generator](https://github.com/GongDexing/data-generator)的基础上改造而来的。

# 使用方法
> src文件中是数据生成工具的源码，你可可以根据自己的需求构建maven工程或者普通的java工程
## 数据生成工具中主要包含两个主要的配置文件：jobs.xml和config.properties
> jobs.xml中主要包含数据库表的配置信息

```
<?xml version="1.0" encoding="UTF-8"?>
<root>
    <jobs>
        <job>
            <id>1</id>
            <num>3</num>
            <table>test</table>
            <filename>test.sql</filename>
            <detail>
                <author>$var{author}</author>
                <id>$key</id>
                <hex>$rule{hex,10}</hex>
		<name>$pool{name}</name>
            </detail>
        </job>
    </jobs>
</root>
```
> config.properties中主要包含一些数据生成策略

```
#内置变量
var.author=gdx
#内置类型
rule.bin=01
rule.hex=0123456789abcdef
rule.letter=abcdefghijklmnopqrstuvwxyz
rule.string=0123456789abcdefghijklmnopqrstuvwxyz
##内置池
pool.name=张三,李四,王五,王八
pool.sex=男,女
```
### $var{variable} 用法
在上面的 jobs.xml 中 $var{author} 代表引用 author 这个变量，在config.properties中进行了配置
> var.author=gdx
### $rule{type,len} 用法
$rule{type,len} 中间是没有任何空格的，在jobs.xml中 $rule{hex,10} 代表从 hex 类型的数据中随机取出10个， hex 在 config.properties 中进行了定义
> rule.hex=0123456789abcdef
### $rule{type} 用法，生成一定范围内的数据
假设id列需要在1~1000之间随机产生，那么在config.properties文件中设置rule.id=1000即可
### $key 用法
$key 是适用于主键列，并且有自增需求的表，如果配置了$key，那么对应的列的值就会从1开始以递增的方式生成
### $pool{name} 用法
$pool{name} 用法和 $var{variable} 类似，本工具中主要是可以在配置的内容中随机的选取一条信息，插入到数据库中。
> pool.name=张三,李四,王五,王八
如果某一列配置了$pool{name}，那么系统就会从 {张三,李四,王五,王八} 这四个人中随机的选取一个。

# 高级使用方法
运行下面命令
> java -jar data-generator.jar

会出现以下提示信息
```
INFO :
           Created By wanglei244@huawei.com
USAGE :
Command line options :

   -h/--ip         Database host, default localhost.                      
   -p/--port       Database port, default 3306.                           
   -c/--thread_num Max connections, default 200                           
   -p/--password   Password, default is empty string.                     
   -D/--database   Database, default ec.                                  
   -j/--jobfile    Job files, default jobs.xml.                           
   -t/--out_put_file_type Out put file type, default none, support type 'csv'.   
   -S              Separators for each record,default is empty string.    
   -C/--configfile Configure files, default config.properties.            
   -i/--insert_or_repalce Insert or Replace, two choice 'insert' or 'replave',default replace.
   -O/--if_database_operation Open or close database operation,two choice 'open' or 'close', default open.
SAMPLE :
java -jar data-generator -h192.168.1.234 -p3306 -Dec -c500 -j jobs.xml -C config.properties -i insert
java -jar data-generator -h192.168.1.234 -O close
java -jar data-generator.jar -O close -t csv
java -jar data-generator.jar -O close -t csv -S space
java -jar data-generator.jar -O close -t csv -S comma

```
### 可以通过命令行配置项，配置需要插入的数据库，并且可以通过-i选项选取插入的方式，默认是replace的方式，你也可以选择insert的方式。
### -c选项是配置数据插入线程的个数，也就是数据库连接的个数。
如果只是进行数据生成并不需要插入数据库请使用:
java -jar data-generator -O close -t csv

注：如果想获取跟多详细信息，请访问[Data-generator](https://github.com/GongDexing/data-generator)

