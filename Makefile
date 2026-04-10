CC=gcc
CFLAGS=-Wall -O3 -g
DAEMON_OUT=tstbd
SO_OUT=tstbc.so
CLI_OUT=tstbcli

COMMON_OBJS=src/tstb-common.o
DAEMON_OBJS=src/daemon/main.o src/daemon/tstb.o
SO_OBJS=src/so/send.o
CLI_OBJS=src/cli/main.o

src/so/%.o: src/so/%.c
	$(CC) -c -o $@ $< $(CFLAGS) -fpic

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)
%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

all: daemon cli so

daemon: $(DAEMON_OBJS) $(COMMON_OBJS)
	$(CC) -o $(DAEMON_OUT) $^ $(CFLAGS)

so: $(SO_OBJS) $(COMMON_OBJS)
	$(CC) -shared -o $(SO_OUT) $^ $(CFLAGS)

cli: $(CLI_OBJS) $(COMMON_OBJS)
	$(CC) -o $(CLI_OUT) $^ $(CFLAGS)

clean:
	rm $(COMMON_OBJS) $(DAEMON_OBJS) $(SO_OBJS) $(CLI_OBJS) $(DAEMON_OUT) $(SO_OUT) $(CLI_OUT)
