/* 
Copyright 2020 Caravan Electronic Publishing, LLC

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


*/

#include <stdio.h>
#include <pcre.h>
#include <string.h>
#include <syslog.h>
#include "list.h"
#include "wrapper.h"

/* 
   input is a list of regex strings 
   output is a list of compiled regex
*/

regex_rule* apply_rules(list* rules, char* command)
{
  
  pcre* new_cre;
  node* n;
  regex_rule* current;
  const char *error;
  int erroroffset;
  int offsets[(MAX_CAP_GROUPS+1)*3]; /*  (max_capturing_groups+1)*3 */
  int offsetcount;

  
  for (n=list_head(rules); n != NULL; n=list_next(rules)){
    current=(regex_rule *)(n->data);
    /*    printf("rulename=%s|subject=%s|flags=%d\n", current->rulename, current->subject,current->flags); */

    if(current->flags & RULE_DISABLED){
      continue;
    }
    
    if( (new_cre = pcre_compile( current->subject, PCRE_DUPNAMES | PCRE_UTF8,
				 &error, &erroroffset, NULL )) == NULL ){
      syslog(LOG_ERR, "regex compilation error \'%s\' for rule \'%s\' \n", error, current->id);
      fprintf(stderr, "WARNING, regex compilation error of rule %s. Please contact the system administrator.\n", current->id );
      exit(1);
    }

    if((offsetcount = pcre_exec(new_cre, NULL, command, strlen(command), 0, 0, offsets, (MAX_CAP_GROUPS+1)*3)) > 0){
      if( current->flags & RULE_LOGALWAYS ){
	syslog(LOG_NOTICE, "\'%s\' matched rule id %s", command, current->id );
      }
      if( current->flags & RULE_LOGDEBUG ){
	syslog(LOG_DEBUG, "\'%s\' matched regex \'%s\'", command, current->subject);
      }
      if( current->flags & RULE_SQUELCH ){
#ifdef TESTONLY
	printf("Rule %s with RULE_SQUELCH flag set was matched. Further regex are being skipped\n", current->rulename);
#endif
	closelog();
	exit(0);
      }
      return(current);
    }else{
      if( current->flags & RULE_LOGDEBUG ){
	syslog(LOG_DEBUG, "\'%s\' did not match regex \'%s\'", command, current->subject);
      }
    }
  }

  /* also true for case where rules is a zero length list */
  return(NULL);
}
