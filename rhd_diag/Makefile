CC=gcc
CFLAGS=-I . -Wall -Werror 
DEPS = # nothing
binaries = rhd2216_util

rhd2216_util:
	mkdir -p ./build
	$(CC) -o ./build/rhd2216_util rhd2216_lib.c pi_spi_lib.c rhd2216_util.c $(CFLAGS)

.PHONY: clean

clean:
	rm -f ./build/$(binaries) ./build/*.o *.o