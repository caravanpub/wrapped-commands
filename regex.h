#ifndef REGEX_H
#define REGEX_H

#include "list.h"
#define MAX_CAP_GROUPS 100

regex_rule* apply_rules(list *rules, char* command);

#endif
