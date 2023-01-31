.PHONY: all clean

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Werror -D_XOPEN_SOURCE=700 -g

all: inversion

clean:
	rm -f inversion inversion.o


inversion: inversion.o
	$(CC) $(CFLAGS) -o inversion inversion.o

inversion.o: inversion.c
	$(CC) $(CFLAGS) -c inversion.c





