/* 
Copyright 2020 Caravan Electronic Publishing, LLC

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#ifndef RULES_H
#define RULES_H

#include "list.h"

/* 
   it's not enough to have one regex for each part of the args being checked,
   e.g. some options may only be suitable with some objects, so 
   we need a list of regex rules for each command.
*/

#define ANYFL "-[[:alnum:]]{0,}"

/* stop processing ALLOWED rule list if rule is matched */
#define RULE_FINAL 1
/* always log when this rule matches */
#define RULE_LOGALWAYS 2
/* add debugging info via syslog() */
#define RULE_LOGDEBUG 4
/* suppress logging and execution, i.e. no no-ops like rm -f without args */
#define RULE_SQUELCH 8
/* rule if defined, but regex processing is disabled */
#define RULE_DISABLED 16

typedef struct REGEX_RULE {
  char* subject;
  char* rulename;
  char* id;
  int flags;
} regex_rule;

/* 
   new_rule(): create a new regex_rule and return the pointer to it. 

   returns a pointer to a regex_rule or NULL if a real_user_list is provided and the 
   real uid of the process does not match an element of the list. 
*/

regex_rule* new_rule(char* subject, char* rulename, char* id, int flags, list* real_user_list);

#endif

