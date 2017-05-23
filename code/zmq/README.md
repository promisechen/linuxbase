
* 编译zeromq 

    tar zxf zeromq-4.0.3.tar.gz   
    cd zeromq-4.0.3   
    ./configure   
    make   
    make install   
    将安装到/usr/local/lib/中。   
    在/etc/ld.so.conf.d定义一个文件，执行ldconfig   

* req-rep 
  s_rep.c c_req.c是req-rep的实现方式  ,服务器端的Ip直接修改c_req.c.
  执行make进行编译 
  在服务器端启动s_rep xxx.txt
  在客户端启动c_req xxx.txt
