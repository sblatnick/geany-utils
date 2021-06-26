CC=gcc

all:
	$(CC) -c quick-opener.c -fPIC -Wwrite-strings `pkg-config --cflags geany`
	$(CC) quick-opener.o -o quick-opener.so -shared `pkg-config --libs geany`
install:
	cp quick-opener.so ~/.config/geany/plugins/
clean:
	rm -f *.o *.so
