* 实验
  执行./compile.sh进行编译处理
(没有实验so的情况)

* 将符号分离


执行下面的语句，将调试信息表导出到foo.dbg中

objcopy --only-keep-debug foo foo.dbg 

执行下面的语句，将去除调试信息

objcopy --strip-debug foo

最终有两个文件，一个是没有调试信息的程序foo,一个只有调试信息的foo.dbg

* 调试方法

  * 直接启动，gdb -e foo -s foo.dbg

  * attach方式 

  ```
    gdb -s 
    attach 34390
    file /xx/xx/foo.dbg
    输入y
    
  ```
或  

  ```
    gdb -s /xx/xx/.foo.dbg
    attatch 34390
    输入n // 输入y,就会被foo给替换成空了。 

  ```
