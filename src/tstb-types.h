#ifndef TSTB_TYPES
#define TSTB_TYPES

enum state {
	STATE_RUNNING,
	STATE_STOPPED
};

enum rule {
	INVALID,
	DONT_MANAGE,
	STOP,
	WAKE_ON_SOCK
};

enum ipc_code {
	IPC_SEND_ALL,
	IPC_SEND_PID,
	IPC_SEND_NAM,
	IPC_REG_PROC,
	IPC_REG_RULE
};

struct config_rule {
	char *name;
	enum rule rule;
};

struct process {
	int pid;
	enum rule rule;

};

typedef int status;
#define OK 0
#define CANT_OPEN_FILE 1
#define OOM 2
#define NOT_OURS 3

#define CONF_INIT_SIZE 8
#define PROC_INIT_SIZE 8
#define FAIL_INIT_SIZE 4

#endif
