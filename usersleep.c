#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>


enum states {
	STATE_RUNNING,
	STATE_STOPPED
};

typedef int rule;

#define INVALID      0b0
#define DONT_MANAGE  0b1
#define STOP         0b10
#define WAKE_ON_SOCK 0b100

struct config_rule {
	char *name;
	rule rules;
};

struct process {
	int pid;
	rule rules;
};

typedef int status;
#define OK 0
#define CANT_OPEN_FILE 1
#define OOM 2
#define NOT_OURS 3

#define CONF_INIT_SIZE 8
#define PROC_INIT_SIZE 8
#define FAIL_INIT_SIZE 4


rule default_rule;
int config_rules_count, config_rules_size;
struct config_rule **config_rules;

int proc_arr_count, proc_arr_size;
struct process **proc_arr;

#define sleep_all(failures) send_all(SIGSTOP, failures)
#define resume_all(failures) send_all(SIGCONT, failures)
int send_all(int signal, struct process **failures){
	int fail_i = 0;
	int failures_size = FAIL_INIT_SIZE;
	failures = malloc(sizeof(struct process) * FAIL_INIT_SIZE);
	for (int i = 0 ; i < proc_arr_count ; i++){
		struct process *proc = proc_arr[i];
		if (proc->rules & STOP){
			kill(proc->pid, signal);
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

int get_rules_for(char *proc_name){
	int l = 0;
	int r = config_rules_count;
	while (l < r){
		int m = l + (r - l) / 2;

		if (!strcmp(config_rules[m]->name, proc_name)){
			return config_rules[m]->rules;
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

status register_proc(int pid, char *path){
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
	proc->rules = get_rules_for(proc_name);
	proc_arr_count++;
	return OK;
}

status register_proc_if_owned(int pid){
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
		struct config_rule *rule = malloc(sizeof(rule));
		if (!rule){
			return OOM;
		}
		config_rules[config_rules_count] = rule;
		config_rules_count++;

		// fill out the rule
		rule->name = target;
		rule->rules = get_rule_flag_for(action);
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

	config_rules = malloc(sizeof(rule) * CONF_INIT_SIZE);
	if (!config_rules){
		return OOM;
	}
	config_rules_count = 0;
	config_rules_size = CONF_INIT_SIZE;

	// first, register a rule to ignore ourselves
	// register_rule("usersleep", "ignore");

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

int init(){
	load_config("/home/hue/.config/usersleep.ini");

	proc_arr = malloc(sizeof(struct process) * PROC_INIT_SIZE);
	if (!proc_arr){
		return OOM;
	}
	proc_arr_count = 0;
	proc_arr_size = PROC_INIT_SIZE;

	// add your pids
	register_proc_if_owned();
	register_proc_if_owned();
	struct process **failures;
	sleep_all(failures);
	sleep(10);
	resume_all(failures);
	
	return OK;
}
