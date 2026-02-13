#include <string.h>

#include "tstb-types.h"

enum rule get_rule_for(char *action){
	#define match(config, rule) if (!strcmp(action, config)){ return rule; }
	match("ignore", DONT_MANAGE)
	match("stop", STOP)
	match("wake_on_sock", WAKE_ON_SOCK)
	#undef match
	return INVALID;
}

