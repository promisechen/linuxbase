main: libadddec.so.1.0 main.c
	$(CC) main.c -g -L. -ladddec -I. -o main
libadddec.so.1.0: add.o dec.o
	$(CC) -shared -g -Wl,-soname,libadddec.so.1 -o libadddec.so.1.0 add.o dec.o
	ln -s libadddec.so.1.0 libadddec.so
%.o: %.c
	gcc -fPIC -g -c $<
clean:
	rm *.o
	rm *.so.*
	rm *.so
	rm main
