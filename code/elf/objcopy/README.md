# 如何将资源文件打入程序中

testfile是资源文件，objcopy.c演示如何引用。  

执行如下命令，将testfile转化成elf文件   

objcopy --readonly-text -I binary -O elf64-x86-64 -B i386:x86-64 testfile test.o

执行objdump -x test.o 可以看到如下信息  

```
test.o：     文件格式 elf64-x86-64
test.o
体系结构：i386:x86-64，标志 0x00000010：
HAS_SYMS
起始地址 0x0000000000000000

节：
Idx Name          Size      VMA               LMA               File off  Algn
  0 .data         0000000b  0000000000000000  0000000000000000  00000040  2**0
                    CONTENTS, ALLOC, LOAD, DATA
                    SYMBOL TABLE:
                    0000000000000000 l    d  .data  0000000000000000 .data
                    0000000000000000 g       .data  0000000000000000 _binary_testfile_start
                    000000000000000b g       .data  0000000000000000 _binary_testfile_end
                    000000000000000b g       *ABS*  0000000000000000 _binary_testfile_size

```
_binary_testfile_分别表示了变量得开始、结束、大小,在程序中可以直接引用这些变量。     
_binary_testfile_start就可以认为是字节得开始位置，如objcopy.c中extern char _binary_testfile_start[]得引用。    
-I -O 得参数是bfdname,执行objcopy最后两行会输出,我这个版本中有这些bfdname     
```
elf64-x86-64 elf32-i386 elf32-x86-64 a.out-i386-linux pei-i386 pei-x86-64 elf64-l1om elf64-k1om elf64-little elf64-big elf32-little elf32-big plugin srec symbolsrec verilog tekhex binary ihex
```
-B 的参数是体系结构，但在帮助手册中都没有相关介绍。      
可以执行objdump -s /bin/ls  可以看到体系解构。    
似乎一般可能在 intel 的cpu 64位就是i386:x86-64 32位就是i386
参考文章：
[用objcopy把调试信息放到单独的文件中](http://blog.csdn.net/someonea/article/details/3202409)
[c++ 符号表分离———objcopy(调试信息挂载)](http://blog.csdn.net/cyteven/article/details/13015511)
