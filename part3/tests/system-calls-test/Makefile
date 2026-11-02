obj-m += syscheck.o

KDIR ?= /lib/modules/$(shell uname -r)/build

.PHONY: all clean run

all: syscheck.ko test-syscalls

syscheck.ko: syscheck.c
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

test-syscalls: test-syscalls.c test-syscalls.h
	$(CC) -Wall -Wextra -o $@ test-syscalls.c

run: test-syscalls
	./test-syscalls

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	rm -f test-syscalls
