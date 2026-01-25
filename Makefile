ODIR=obj
CC=gcc
CFLAGS=-Wall -O3

OBJS=main.o usersleep.o

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

build: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)
