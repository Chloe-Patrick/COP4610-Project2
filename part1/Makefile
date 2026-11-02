CC = gcc
CFLAGS = -Wall -Wextra -O0

.PHONY: all traces verify clean

all: empty part1

empty: empty.c
	$(CC) $(CFLAGS) -o $@ $<

part1: part1.c
	$(CC) $(CFLAGS) -o $@ $<

traces: all
	strace -o empty.trace ./empty
	strace -o part1.trace ./part1

verify: traces
	@a=$$(grep -Ec '^[a-z_][a-z_0-9]*\(' empty.trace || :); \
	b=$$(grep -Ec '^[a-z_][a-z_0-9]*\(' part1.trace || :); \
	echo "empty: $$a system calls, part1: $$b system calls"; \
	echo "Difference: $$((b-a))"; \
	test $$((b-a)) -eq 5

clean:
	rm -f empty part1
