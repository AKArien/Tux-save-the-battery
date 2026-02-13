#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <sys/un.h>
#include <sys/socket.h>

#include "../tstb-types.h"
#include "../tstb-common.h"

int main(int argc, char *argv[]){
	char *path = "/tmp/tstbd.sock";

	#define BUF_SIZE 100
	char buf[BUF_SIZE];
	#undef BUF_SIZE
	int pos = 0;

	if (strcmp(argv[1], "send-all") == 0){
		if (argc < 3){
			fprintf(stderr, "Usage: send-all <signal>\n");
			return 1;
		}
		buf[pos++] = IPC_SEND_ALL;
		buf[pos++] = atoi(argv[2]);
	}
	else if (strcmp(argv[1], "send-by-pid") == 0){
		if (argc < 4){
			fprintf(stderr, "Usage: send-by-pid <pid> <signal>\n");
			return 1;
		}
		buf[pos++] = IPC_SEND_PID;

		unsigned pid = atoi(argv[2]);
		for (int a = 0; a < sizeof(int); a++){
		buf[pos++] = (pid >> (a * CHAR_BIT));
		}
		buf[pos++] = atoi(argv[3]);
	}
	else if (strcmp(argv[1], "send-by-nam") == 0){
		if (argc < 4){
			fprintf(stderr, "Usage: send-by-nam <name> <signal>\n");
			return 1;
		}
		buf[pos++] = IPC_SEND_NAM;

		size_t name_len = strlen(argv[2]) + 1;
		memcpy(buf + pos, argv[2], name_len);
		pos += name_len;
		buf[pos++] = atoi(argv[3]);
	}
	else if (strcmp(argv[1], "reg-proc") == 0){
		if (argc < 3){
			fprintf(stderr, "Usage: reg-proc <pid>\n");
			return 1;
		}
		buf[pos++] = IPC_REG_PROC;

		unsigned pid = atoi(argv[2]);
		for (int a = 0; a < sizeof(int); a++){
			buf[pos++] = (pid >> (a * CHAR_BIT));
		}
	}
	else if (strcmp(argv[1], "reg-rule") == 0){
		if (argc < 4){
			fprintf(stderr, "Usage: reg-rule <target> <action>\n");
			return 1;
		}
		buf[pos++] = IPC_REG_RULE;

		size_t target_len = strlen(argv[2]) + 1;
		memcpy(buf + pos, argv[2], target_len);
		pos += target_len;

		buf[pos++] = (char)get_rule_for(argv[3]);
	}
	else {
		fprintf(stderr, "Unknown command: %s\n", argv[1]);
		return 1;
	}

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
		return 1;

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path) -1);

	if (connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
		return 2;

	write(sock, buf, pos);
	close(sock);

	exit(EXIT_SUCCESS);

	return 0;
}
