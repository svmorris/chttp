CC = gcc
CFLAGS_DEBUG = -g -Wall -DDEBUG
CFLAGS_RELEASE = -O2 -Wall
CFLAGS_DEBUG_STRICT = -g -Wall -DDEBUG -Werror -Wextra -Wuninitialized

server: main.c
	$(CC) $(CFLAGS_RELEASE) -o $@ $<

.PHONY: debug

debug:
	$(CC) $(CFLAGS_DEBUG) -o server main.c

.PHONY: strict

sdebug:
	$(CC) $(CFLAGS_DEBUG_STRICT) -o server main.c

clean:
	rm -f server
