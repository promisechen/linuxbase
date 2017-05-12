###
# @file
# @brief Makefile
# @date 20170513
#

#ar
AR = ar

#C编译器
CC = gcc
#C++编译器
CXX = g++
#编译模式,默认使用default

sources=$(wildcard *.c)
objs=$(sources:.c=.o)
target=pobjcopy

CXXFLAGS += 
LDFLAGS += 

$(target): $(objs)
	objcopy --readonly-text -I binary -O elf64-x86-64 -B i386:x86-64 testfile test.o
	$(CC) -o $@ $^ test.o $(LDFLAGS) $(CXXFLAGS)
$(objs): %.o: %.c 
	$(CC) -o $@ -c $< $(CXXFLAGS)

install:
	
uninstall:
	
clean:
	rm $(objs) $(target) -fr

