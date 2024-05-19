CC=gcc
CFLAGS=-I . -Wall -Werror 
DEPS = # nothing
binaries = rhd2216_util

rhd2216_util:
	mkdir -p ./build
	# -g for debug info 
	$(CC) -g rhd2216_lib.c pi_spi_lib.c rhd2216_util.c $(CFLAGS) -o ./build/rhd2216_util 

.PHONY: clean

clean:
	rm -f ./build/$(binaries) ./build/*.o *.o