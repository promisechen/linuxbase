gcc -g -o foo foo.c
objcopy --only-keep-debug foo foo.dbg 
objcopy --strip-debug foo
