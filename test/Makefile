EXEC ?= memalloc_test
CROSS_COMPILE ?= aarch64-linux-gnu-
SYSROOT ?=

$(EXEC): main.c
	$(CROSS_COMPILE)gcc -I$(SYSROOT)/usr/include -I../dev/ -g -O3 -o $@ $^

clean:
	rm -f $(EXEC)


