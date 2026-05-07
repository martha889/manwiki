CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -O2

.PHONY: all clean

all: manwiki

manwiki: main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f manwiki
