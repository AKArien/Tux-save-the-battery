#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../ipc-codes.h"

enum states {
	STATE_RUNNING,
	STATE_STOPPED
};

enum rule {
	INVALID,
	DONT_MANAGE,
	STOP,
	WAKE_ON_SOCK
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


enum rule default_rule;
int config_rules_count, config_rules_size;
struct config_rule **config_rules;

int proc_arr_count, proc_arr_size;
struct process **proc_arr;


int apply(struct process *proc, int signal){
	switch (proc->rule){
		case STOP:
			return kill(proc->pid, signal);
		break;
	}
}

int apply_by_pid(int pid, int signal){

}

int apply_by_name(char *name, int signal){
	
}

#define sleep_all(failures) apply_all(SIGSTOP, failures)
#define resume_all(failures) apply_all(SIGCONT, failures)
int apply_all(int signal, struct process **failures){
	int fail_i = 0;
	int failures_size = FAIL_INIT_SIZE;
	failures = malloc(sizeof(struct process) * FAIL_INIT_SIZE);

	for (int i = 0 ; i < proc_arr_count ; i++){
		struct process *proc = proc_arr[i];
		if (!apply(proc, signal)){
			continue;
		}

		if (!(fail_i >= failures_size)){
			struct process **mem = realloc(failures, failures_size*2);
			if (!mem){
				continue;
			}
			failures = mem;
			failures_size *= 2;
		}
		failures[i] = proc_arr[i];
		fail_i++;
	}
	return fail_i;
}

enum rule get_rules_for(char *proc_name){
	int l = 0;
	int r = config_rules_count;
	while (l < r){
		int m = l + (r - l) / 2;

		if (!strcmp(config_rules[m]->name, proc_name)){
			return config_rules[m]->rule;
		}

		int i = 0;
		// segfault if one is a superset of the other, fix that shit
		while (proc_name[i] == config_rules[m]->name[i]){
			i++;
		}

		if (config_rules[m]->name[i] < proc_name[i])
			l = m + 1;
		else
		    r = m - 1;

	}

	return default_rule;
}

int is_user_process(FILE *fd){
	struct stat buf;
	if (fstat(fileno(fd), &buf) == -1){
		return 0;
	}
	if (buf.st_uid == geteuid()){
		return 1;
	}
	return 0;
}

status register_proc(unsigned pid, char *path){
	if (proc_arr_count >= proc_arr_size){
		struct process **mem = realloc(proc_arr, proc_arr_size*2);
		if (!mem){
			return OOM;
		}
		proc_arr = mem;
		proc_arr_size *= 2;
	}
	struct process *proc = malloc(sizeof(struct process));
	if (!proc){
		return OOM;
	}
	proc_arr[proc_arr_count] = proc;
	proc->pid = pid;
	strcat(path, "/cmdline");
	FILE *cmdline = fopen(path, "r");
	if (!cmdline){
		free(proc);
		return CANT_OPEN_FILE;
	}
	char proc_name[100];
	fgets(proc_name, 100, cmdline);
	fclose(cmdline);
	proc->rule = get_rules_for(proc_name);
	proc_arr_count++;
	return OK;
}

status register_proc_if_owned(unsigned pid){
	char num[7];
	sprintf(num, "%d", pid);
	char path[6/*/proc/*/+ 7/*num*/+ 7/*cmdline*/] = "/proc/";
	strcat(path, num);

	FILE *fd = fopen(path, "r");
	if (!fd){
		return CANT_OPEN_FILE;
	}
	if (!is_user_process(fd)){
		fclose(fd);
		return NOT_OURS;
	}
	fclose(fd);

	return register_proc(pid, path);
}

status register_all_procs(){

}

int get_rule_flag_for(char *string){
	int flags = INVALID;
	#define match(config, flag) if (!strcmp(string, config)){ flags = flags | flag; }
	// if (!strcmp(string, "ignore")){
	// 	flags = flags | DONT_MANAGE;
	// }
	match("ignore", DONT_MANAGE)
	match("stop", STOP)
	match("wake_on_sock", WAKE_ON_SOCK)
	#undef match
	return flags;
}

status register_rule(char *target, char *action){
	if (!strcmp(target, "default")){
		default_rule = get_rule_flag_for(action);
	}
	else {
		// append the rule to the end of config_rules, the caller is expected to sort after adding the rules
		if (config_rules_count > config_rules_size){
			struct config_rule **mem = realloc(config_rules, config_rules_size * 2);
			if (!mem){
				return OOM;
			}
			config_rules = mem;
			config_rules_size *= 2;
		}
		struct config_rule *rule = malloc(sizeof(enum rule));
		if (!rule){
			return OOM;
		}
		config_rules[config_rules_count] = rule;
		config_rules_count++;

		// fill out the rule
		rule->name = target;
		rule->rule = get_rule_flag_for(action);
	}
	return OK;
}

char *trim_spaces(char *str){
	char *end;
	while (isspace((unsigned char)*str)) str++;

	if (*str == 0)
		return str;

	end = str + strlen(str) -1;
	while (end > str && isspace((unsigned char)*end)) end--;
	end[1] = '\0';
	return str;
}

status load_config(char *path){
	// read .ini config file
	// insert into default, config_rules
	// sort config_rules
	FILE *fd = fopen(path, "r");
	if (!fd){
		puts("couldn’t open config file");
		return CANT_OPEN_FILE;
	}

	config_rules = malloc(sizeof(enum rule) * CONF_INIT_SIZE);
	if (!config_rules){
		return OOM;
	}
	config_rules_count = 0;
	config_rules_size = CONF_INIT_SIZE;

	while (1){
		#define BUF_SIZE 100
		char *buf = malloc(BUF_SIZE * sizeof(char));
		int realloc_count = 1;
		if (!buf){
			return OOM;
		}
		size_t limit = realloc_count * BUF_SIZE;
		if (getline(&buf, &limit, fd) == 0 || !strcmp(buf, "")){
			free(buf);
			break;
		}
		while (strchr(buf, '\n') == NULL){
			char *mem = realloc(buf, BUF_SIZE * ++realloc_count);
			if (!mem){
				return OOM;
			}
			buf = mem;
			getline(&buf, &limit, fd);
		}
		char *target = strsep(&buf, "=");
		char *tt = trim_spaces(target);
		char *bt = trim_spaces(buf);
		// buf now holds the action
		register_rule(tt, bt);
		// free(buf);
		#undef BUF_SIZE
	}

	fclose(fd);
	return OK;
}

int tstb_daemon(char *path){
	#define BUF_SIZE 100
	char buf[BUF_SIZE];

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
		return 99;

	if ((-1 == remove(path)) && (errno != ENOENT)){
		return 98;
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path) -1);
	if (bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1){
		perror("bind failed");
		return 97;

	}

	listen(sock, 1);

	for (;;){
		int con = accept(sock, NULL, NULL);
		if (con == -1)
			continue;

		int count = 0;
		ssize_t size;
		while ((size = read(con, buf, BUF_SIZE)) > 0){
			count += size;
			write(STDOUT_FILENO, buf, size);
			// handle input being bigger than buf
		}

		for (int i = 0 ; i < count ; i++){
			// this only handles happy case, fix that someday
			unsigned pid = 0;
			switch (buf[i]){
				case IPC_SEND_ALL:
					struct process **failures;
					apply_all(buf[++i], failures);
				break;

				case IPC_SEND_PID:
					// int pid = 0;
					for (int a = 0 ; a < sizeof(int) ; a++){
						pid = pid | (unsigned char)buf[++i] << (a * CHAR_BIT);
					}
					apply_by_pid(pid, (int)buf[++i]);
				break;

				case IPC_SEND_NAM:
					char *name_end = strchr(buf + i + 1, '\0');
					apply_by_name(buf + i, (int)name_end + 1);
					i = (int)(name_end - buf);
				break;

				case IPC_REG_PROC:
					// int pid = 0;
					for (int a = 0 ; a < sizeof(int) ; a++){
						pid = pid | (unsigned char)buf[++i] << (a * CHAR_BIT);
					}
					register_proc_if_owned(pid);
				break;

				case IPC_REG_RULE:
					char *target_end = strchr(buf + i, '\0'); 
					char *action_end = strchr(target_end + 1, '\0');
					register_rule(buf + i, target_end + 1);
					i = (int)(action_end - buf);
				break;
			}
		}


		close(con);
		
	}

	#undef BUF_SIZE
}

int init(){
	load_config("/home/hue/.config/tstb.ini");

	proc_arr = malloc(sizeof(struct process) * PROC_INIT_SIZE);
	if (!proc_arr){
		return OOM;
	}
	proc_arr_count = 0;
	proc_arr_size = PROC_INIT_SIZE;

	tstb_daemon("/tmp/tstbd.sock");
	
	return OK;
}
