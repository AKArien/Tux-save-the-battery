CC=gcc
CFLAGS=-Wall -O3 -g
DAEMON_OUT=tstbd
CLIENT_OUT=tstbc

DAEMON_OBJS=src/daemon/main.o src/daemon/tstb.o
CLIENT_OBJS=src/client/main.o

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

all: daemon client

	
daemon: $(DAEMON_OBJS)
	$(CC) -o $(DAEMON_OUT) $^ $(CFLAGS)

client: $(CLIENT_OBJS)
	$(CC) -o $(CLIENT_OUT) $^ $(CFLAGS)

clean:
	rm $(DAEMON_OBJS) $(CLIENT_OBJS) $(DAEMON_OUT) $(CLIENT_OUT)
