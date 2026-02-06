ODIR=obj
CC=gcc
CFLAGS=-Wall -O3 -g
OUT_NAME=tstb

OBJS=main.o tstb.o

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

build: $(OBJS)
	$(CC) -o $(OUT_NAME) $^ $(CFLAGS)

clean:
	rm $(OBJS) $(OUT_NAME)
