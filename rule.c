/* 
Copyright 2020 Caravan Electronic Publishing, LLC

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


*/

/* 
   it's not enough to have one regex for each part of the args being checked,
   e.g. some options may only be suitable with some objects, so 
   we need a list of regex rules for each command.
*/
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pwd.h>
#include "rule.h"

/* return NULL if real_user_list is NOT NULL and SUDO_USER not in list */

regex_rule* new_rule(char* subject, char* rulename, char* id, int flags, list* real_user_list)
{
  regex_rule* rule;
  node* n;
  char* real_user;
  struct passwd *userinfo;

  if( real_user_list != NULL ){
    if((real_user=getenv("SUDO_USER")) == NULL){
      /* there some doubt as to what getlogin() does in all cases */
      userinfo = getpwuid(getuid());
      real_user=userinfo->pw_name;
    }
    for(n=list_head(real_user_list); n!=NULL; n=list_next(real_user_list)){
      if(strcmp(real_user, (char *)(n->data)) == 0){
	break;
      }
    }
    if(n==NULL){ /* we did not match any username in real_user_list */
#ifndef TESTONLY
      /* just keep processing in simulator mode since the users doesn't have to exist on this system */
      return(NULL);
#endif
      
    }
  }
  
  if((rule=malloc(sizeof(regex_rule))) == NULL){
    perror("new_rule");
  }
  rule->subject=malloc(strlen(subject) + 1);
  rule->rulename=malloc(strlen(rulename) + 1);
  rule->id=malloc(strlen(id) + 1); 
  strcpy(rule->subject, subject);
  strcpy(rule->rulename, rulename);
  strcpy(rule->id, id);
  rule->flags=flags;

  return(rule);
}
