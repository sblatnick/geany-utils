CC=gcc
PLUGINS   = $(wildcard *.c)
TARGETS   = $(patsubst %.c, %, $(PLUGINS))

list:
	@ls -1 *.c | sed 's/\.c$$//'
$(TARGETS):
	@echo "Building $@"
	$(CC) -c $@.c -fPIC -Wwrite-strings `pkg-config --cflags geany`
	$(CC) $@.o -o $@.so -shared `pkg-config --libs geany`
install:
	cp *.so ~/.config/geany/plugins/
clean:
	rm -f *.o *.so
