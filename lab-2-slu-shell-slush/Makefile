CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

all: slush

slush: slush.c
	$(CC) $(CFLAGS) -o slush slush.c

clean:
	rm -f slush

.PHONY: all clean
