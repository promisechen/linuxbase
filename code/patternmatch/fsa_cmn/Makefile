
sources=$(wildcard *.c)
objs=$(sources:.c=.o)
target=fsatest

#CFLAGS = -Wall -g -O3
CFLAGS = -Wall -g
LDFLAGS =

.PHONY: clean

$(target): $(objs)
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

$(objs): %.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf $(objs) $(target)


