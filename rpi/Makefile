CC=gcc
CFLAGS=-I . -Wall -Werror 
DEPS = # nothing
binaries = rhd2216_util

rhd2216_util:
	$(CC) -o rhd2216_util rhd2216_lib.h rhd2216_lib.c rhd2216_util.c $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(binaries) *.o