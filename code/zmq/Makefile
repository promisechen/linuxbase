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

CXXFLAGS += -lzmq
LDFLAGS += 

$(target): 
	$(CC) -o s_dealer s_dealer.c $(CXXFLAGS)
	$(CC) -o c_dealer c_dealer.c $(CXXFLAGS)
	$(CC) -o s_rep s_rep.c $(CXXFLAGS)
	$(CC) -o c_req c_req.c $(CXXFLAGS)

install:
	
uninstall:
	
clean:
	rm $(objs) $(target) s_rep c_req c_dealer s_dealer -fr

