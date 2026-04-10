#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "../tstb-types.h"
#include "../tstb-common.h"

struct tstbc_buf {
	char *buf;
	unsigned size;
	unsigned pos;
};

// execute the payload. This frees the buffer.
int tstbc_send(struct tstbc_buf *buf){
	char *path = "/tmp/tstbd.sock";

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
		return 1;

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path) -1);

	if (connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
		return 2;

	write(sock, buf->buf, buf->pos);
	close(sock);

	free(buf->buf);
	free(buf);

	exit(EXIT_SUCCESS);

	return 0;
}


#define INIT_BUF_SIZE 10

struct tstbc_buf *tstbc_create_buf(){

	struct tstbc_buf *buf = malloc(sizeof(struct tstbc_buf));
	if (!buf)
		return NULL;

	buf->buf = malloc(sizeof(char) * INIT_BUF_SIZE);
	if (!buf->buf){
		free(buf);
		return NULL;
	}

	buf->size = INIT_BUF_SIZE;
	buf->pos = 0;

	return buf;
}

// macro to realloc buf.buf if we would go beyond. makes caller return 1 if realloc failed
#define extend_buf_if_needed(chars_forward) \
	while (buf->pos + chars_forward > buf->size){ \
		char *m = realloc(buf->buf, buf->size * 2); \
		if (!m) \
			return 1; \
\
		buf->buf = m; \
		buf->size = buf->size * 2; \
	}

int tstbc_add_send_all(struct tstbc_buf *buf, int signal){
	extend_buf_if_needed(2)
	buf->buf[buf->pos++] = IPC_SEND_ALL;
	buf->buf[buf->pos++] = signal;
	return 0;
}

int tstbc_add_send_to_pid(struct tstbc_buf *buf, unsigned pid, int signal){
	extend_buf_if_needed(sizeof(unsigned) + 2)
	buf->buf[buf->pos++] = IPC_SEND_PID;
	for (int i = 0 ; i < sizeof(unsigned) ; i++){
		buf->buf[buf->pos++] = pid >> (pid * CHAR_BIT);
	}
	buf->buf[buf->pos++] = signal;
	return 0;
}

int tstbc_add_send_to_name(struct tstbc_buf *buf, char *name, int signal){
	size_t len = strlen(name);
	extend_buf_if_needed(len + 2)
	buf->buf[buf->pos++] = IPC_SEND_NAM;
	memcpy(buf->buf + buf->pos, name, len);
	buf->pos += len;
	buf->buf[buf->pos++] = signal;
	return 0;
}

int tstbc_add_reg_proc(struct tstbc_buf *buf, unsigned pid){
	extend_buf_if_needed(sizeof(unsigned) + 1)
	buf->buf[buf->pos++] = IPC_REG_PROC;
	for (int i = 0 ; i < sizeof(unsigned) ; i++){
		buf->buf[buf->pos++] = pid >> (pid * CHAR_BIT);
	}
	return 0;
}

int tstbc_add_reg_rule(struct tstbc_buf *buf, char *name, enum rule rule){
	size_t len = strlen(name);
	extend_buf_if_needed(len + 2)
	buf->buf[buf->pos++] = IPC_REG_RULE;
	memcpy(buf->buf + buf->pos, name, len);
	buf->pos += len;
	buf->buf[buf->pos++] = (char)rule;
	return 0;
}

#undef extend_buf_if_needed
#undef INIT_BUF_SIZE
