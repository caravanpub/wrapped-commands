/* 
Copyright 2020 Caravan Electronic Publishing, LLC

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include "my_asprintf.h"
#include "wrapper.h"
#include "rule.h"
#include "list.h"

/* 
   These are just some generic allow rules and some good 
   best practices forbidden rules. You'll need to add rules 
   to implement your local policy. Allow by default or only 
   allow specific things are both easy to do here. Do 
   check the available flags in rule.h to get a better idea 
   as to what's possible.
*/

#define TMP "(/var/tmp|/tmp)"

/* create lists of users if you wish to limit a rule to certain users */
list* root_user_list;
/* list* psuedo_root_user_list;*/

void init_user_lists()
{
  root_user_list = new_list(NULL);
  list_add(root_user_list, new_node("root"));
  /*
  psuedo_root_user_list = new_list(NULL);
  list_add(psuedo_root_user_list, new_node("root1"));
  list_add(psuedo_root_user_list, new_node("build1"));
  */
}

char* sudo_user_homedir(char* sudo_user)
{
  struct passwd *userinfo;
  /* this can't fail... probably */
  userinfo = getpwnam(sudo_user);

  if(userinfo == NULL){
#ifdef TESTONLY
    return(my_asprintf(6 + strlen(sudo_user) + 1, "/home/%s", sudo_user));
#endif
    return("/nonexistent");
  }else{
    return(userinfo->pw_dir);
  }
}

/* here are where you define the allowed and fobidden rules for each wrapped 
   command */

list* rm_allowed()
{
  list* foo;
  char* u; /* sudo user name */
  char* fee;
  foo=new_list(NULL);

  init_user_lists();

  /*

    Some commented examples of real rules: 

  list_add(foo, new_node(new_rule("^-[rf]{1,2}([[:space:]]{1,}" TMP "/\\.s\\.PGSQL\\.15007(\\.lock){0,1}){1,}$",
				  "FINAL RULE - Allow users to remove postgresql local socket and lockfile, rev 23535",
				  "RM-06",
				  RULE_FINAL,
				  NULL)));


  list_add(foo, new_node(new_rule("^-f[[:space:]]{1,}" TMP "/cpusetlist[[:digit:]]{1,}$",
				  "rev 23535.9",
				  "RM-29",
				  0,NULL)));
  */
  

  if((u=getenv("SUDO_USER")) != NULL){
    list_add(foo, new_node(new_rule((fee=my_asprintf(10240, "^(-[rf]{1,2}[[:space:]]{0,1}){0,1}([[:space:]]{0,}(/home|/export/home)/%s/[-._/+[:alnum:]]{1,}){1,}$", u)),
				    "Dynamic rule to allow rm in real users homedir",
				    "RM-48",
				    0,
				    NULL)));
    free(fee);
  }
  
  if(geteuid() != 0 ){
    list_add(foo, new_node(new_rule(".*",
				    "Allow non-priv users to rm files if otherwise allowed by the system — \
effectively a passthrough for when wrappers end up in a users PATH. Exclude Rules still apply.",
				    "RM-49",
				    0,
				    NULL)));
  }

  if((u=getenv("SUDO_USER")) != NULL){
    list_add(foo, new_node(new_rule((fee=my_asprintf(10240, "%s%s%s", "^((-r|-f|-rf|-fr)[[:space:]]{1,}){0,1}", sudo_user_homedir(u), "[-._/+[:alnum:]]{1,}$")),
				    "Dynamic rule to allow rm (of one arg only) in real users homedir",
				    "RM-51",
				    0,
				    NULL)));
    free(fee);
  }

  list_add(foo, new_node(new_rule(".*",
				  "Allow root to delete files broadly, if not forbidden",
				  "RM-9999",
				  RULE_LOGALWAYS,
				  root_user_list)));

  return(foo);
}

list* rm_forbidden()
{
  list* foo;
  foo=new_list(NULL);
  
  list_add(foo, new_node(new_rule("(\\.\\.|//)",
				  "disallow .. and //",
				  "RMF-01",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(;&)",
				  "disallow command chaining",
				  "RMF-02",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/var/spool(/){0,1}",
				  "protect /var/spool",
				  "RMF-03",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/root/\\.[[:alnum:]]{0,}",
				  "protect /root",
				  "RMF-04",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/etc/",
				  "protect /etc",
				  "RMF-05",
				  RULE_LOGALWAYS,
				  NULL)));

  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/(bin|sbin|usr/bin|usr/sbin|lib(64){0,1}|usr/lib(64){0,1}|libexec|usr/libexec|var/log|var/spool/|var/ossec|opt/tools)",
				  "protect system dirs",
				  "RMF-06",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("/(var/tmp|tmp)/(\\.|screens|vi|ssh-|xf-dll|keyring|s\\.eventmond|ossec|protect|no-del|nodel|no_del)",
				  "protect /tmp from accidental over reach of wrapped delete",
 				  "RMF-07",
				  0,
				  NULL)));
  list_add(foo, new_node(new_rule("(/export/home|/home)/[-._[:alnum:]]{1,}/\\.ssh/(authorized_keys(2){0,1}|config|restricted)",
				  "protect exploitable files in user .ssh dir from deletion",
				  "RMF-08",
				  0,
				  NULL)));
  return(foo);
}

list* cp_allowed()
{
  list* foo;
  char* u;
  char* fee;

  foo=new_list(NULL);

  if((u=getenv("SUDO_USER")) != NULL){
    list_add(foo, new_node(new_rule((fee=my_asprintf(10240, "(/home|/export/home)/%s/[-._/+[:alnum:]]{1,}", u)),
				    "Dynamic rule to allow cp to real users homedir",
				    "CP-18",
				    0,
				    NULL)));
    free(fee);
  }

  if(geteuid() != 0 ){
    list_add(foo, new_node(new_rule(".*",
				    "Allow non-priv users to cp files if otherwise allowed by the system — \
effectively a passthrough for when wrappers end up in a users PATH. Exclude Rules still apply.",
				    "CP-21",
				    0,
				    NULL)));
  }

  if((u=getenv("SUDO_USER")) != NULL){ 
    list_add(foo, new_node(new_rule((fee=my_asprintf(10240, "%s%s%s","^((-p|-r|-rp|-pr)[[:space:]]{1,}){0,1}[-._/+[:alnum:]]{1,}[[:space:]]{1,}", sudo_user_homedir(u), "[-._/+[:alnum:]]{0,}$")),
				    "Allow copy of nearly anything to SUDO_USER home dir",
				    "CP-23",
				    0,
				    NULL)));
    free(fee);
  }

  return(foo);
}

list* cp_forbidden()
{
  list* foo;
  foo=new_list(NULL);
  
  list_add(foo, new_node(new_rule("(\\.\\.|//)",
				  "disallow .. and //",
				  "CPF-01",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(;&)",
				  "disallow command chaining",
				  "CPF-02",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("((?<![[:alnum:]])/root/\\.[[:alnum:]]{0,})",
				  "protect /root",
				  "CPF-03",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/etc/)",
				  "protect /etc",
				  "CPF-04",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/(var/tmp|tmp)/protect",
				  "wrapped functions have no access to /tmp/protect*",
				  "CPF-05",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(shadow|master\\.passwd)",
				  "protect cp of passwd info",
				  "CPF-06",
				  0,
				  NULL)));
  list_add(foo, new_node(new_rule("id_(dsa|rsa)",
				  "protect cp of private keys",
				  "CPF-07",
				  0,
				  NULL)));

  return(foo);
}

list* mkdir_allowed()
{
  list* foo;
  char* fee;
  char* u;
  foo=new_list(NULL);
  
  if((u=getenv("SUDO_USER")) != NULL){
    list_add(foo, new_node(new_rule((fee=my_asprintf(10240, "^(-p[[:space:]]{0,}|-m[[:space:]][0]{0,1}[0-7][0-5][0-5][[:space:]]{0,}){0,2}([[:space:]]{0,}(/home|/export/home)/%s/[-._/+[:alnum:]]{1,}){1,}$", u)),
				    "Dynamic rule to allow mkdir in real users homedir",
				    "MK-14",
				    0,
				    NULL)));
    free(fee);
  }
  if(geteuid() != 0){
    list_add(foo, new_node(new_rule(".*",
				    "Allow non-priv users to mkdir if otherwise allowed by the system — \
effectively a passthrough for when wrappers end up in a users PATH. Exclude Rules still apply.",
				    "MK-15",
				    0,
				    NULL)));
  }

  return(foo);
}

list* mkdir_forbidden()
{
  list* foo;
  foo=new_list(NULL);
  
  list_add(foo, new_node(new_rule("(\\.\\.|//)",
				  "disallow .. and //",
				  "MKF-01",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(;&)",
				  "disallow command chaining",
				  "MKF-02",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/(var/tmp|tmp)/protect[-._/+[:alnum:]]{0,}$",
				  "wrapped functions have no access to /[var/]tmp/protect*",
				  "MKF-03",
				  RULE_LOGALWAYS,
				  NULL)));
  return(foo);
}

list* chmod_allowed()
{
  list* foo;
  char* fee;
  char* u;
  foo=new_list(NULL);

  if((u=getenv("SUDO_USER")) != NULL){
    list_add(foo, new_node(new_rule((fee=my_asprintf(10240, "^(-R ){0,1}[0-7][0-5][0-5][[:space:]]{1,}(/home|/export/home)/%s/[-._/+[:alnum:]]{1,}", u)),
				    "Dynamic rule to allow chmod (of one arg only) in real users homedir",
				    "CHM-19",
				    0,
				    NULL)));
    free(fee);
  }
  if(geteuid() != 0 ){
    list_add(foo, new_node(new_rule(".*",
				    "Allow non-priv users to chmod if otherwise allowed by the system — \
effectively a passthrough for when wrappers end up in a users PATH. Exclude Rules still apply.",
				    "CHM-20",
				    0,
				    NULL)));
  }

  if((u=getenv("SUDO_USER")) != NULL){ 
    list_add(foo, new_node(new_rule((fee=my_asprintf(10240, "%s%s%s","^(-R[[:space:]]{1,}){0,1}(0){0,1}[0-7][0-7][0-7][[:space:]]{1,}", sudo_user_homedir(u), "[-._/+[:alnum:]]{0,}$")),
				    "Allow chmod of nearly anything in SUDO_USER home dir, e.g. for snap lib and tool",
				    "CHM-22",
				    0,
				    NULL)));
    free(fee);
  }

  return(foo);
}

list* chmod_forbidden()
{

  list* foo;
  foo=new_list(NULL);

  list_add(foo, new_node(new_rule("(\\.\\.|//)",
				  "disallow .. and //",
				  "CHMF-01",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(;&)",
				  "disallow command chaining",
				  "CHMF-02",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("^[1-7][0-7]{3}[[:space:]]",
				  "no octal setuid of gid bit",
				  "CHMF-03",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("^(a|u|g|o)(\\+|=)[rwxXugo]*(s{1,}|t{1,})[rwxXugo]*",
				  "no symbolic setuid or sticky bits",
				  "CHMF-04",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("[0-7]{3,4}([[:space:]]{1,}/etc/(shadow|master.passwd|passwd|group|profile|hosts|nsswitch.conf|resolv.conf|exports|fstab|auto.master|login.conf|passwd.conf)){1,}$",
				  "CHMOD protect /etc",
				  "CHMF-05",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/(var/tmp|tmp)/protect[-._/+[:alnum:]]{0,}",
				  "wrapped functions have no access to /tmp/protect*",
				  "CHMF-06",
				  RULE_LOGALWAYS,
				  NULL)));
  return(foo);
  
}

list* chown_allowed()
{
  list* foo;
  char* fee;
  char* u;
  foo=new_list(NULL);

  if((u=getenv("SUDO_USER")) != NULL){
    list_add(foo, new_node(new_rule((fee=my_asprintf(10240, "^%s[[:space:]]{1,}(/home|/export/home)/%s/[-._/+[:alnum:]]{1,}", u, u)),
				    "Dynamic rule to allow limited chown in real users homedir",
				    "CHO-09",
				    0,
				    NULL)));
    free(fee);
  }
  if(geteuid() != 0 ){
    list_add(foo, new_node(new_rule(".*",
				    "Allow non-priv users to chown files if otherwise allowed by the system — \
effectively a passthrough for when wrappers end up in a users PATH. Exclude Rules still apply. NOTE, this is largely \
pointless since a user with euid != 0 can only chown files to themselves, so this is a no op.",
				    "CHO-10",
				    0,
				    NULL)));
  }
  if((u=getenv("SUDO_USER")) != NULL){ 
    list_add(foo, new_node(new_rule((fee=my_asprintf(10240, "%s%s%s%s%s","^(-R[[:space:]]{1,}){0,1}", u, "[[:space:]]{1,}", sudo_user_homedir(u), "[-._/+[:alnum:]]{0,}$")),
				    "Allow chown SUDO_USER of nearly anything in SUDO_USER home dir, e.g. for snap lib and tool",
				    "CHO-12",
				    0,
				    NULL)));
    free(fee);
  }

  return(foo);
}

list* chown_forbidden()
{
  list* foo;
  foo=new_list(NULL);

  list_add(foo, new_node(new_rule("(\\.\\.|//)",
				  "disallow .. and //",
				  "CHOF-01",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(;&)",
				  "disallow command chaining",
				  "CHOF-02",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/(var/tmp|tmp)/protect[-._/+[:alnum:]]{0,}",
				  "wrapped functions have no access to /tmp/protect*",
				  "CHOF-03",
				  RULE_LOGALWAYS,
				  NULL)));
  return(foo);
}

list* chgrp_allowed()
{
  list* foo;
  foo=new_list(NULL);

  if(geteuid() != 0 ){
    list_add(foo, new_node(new_rule(".*",
				    "Allow non-priv users to chgrp if otherwise allowed by the system — \
effectively a passthrough for when wrappers end up in a users PATH. Exclude Rules still apply.",
				    "CHG-04",
				    0,
				    NULL)));
  }

  return(foo);
}

list* chgrp_forbidden()
{
  list* foo;
  foo=new_list(NULL);

  list_add(foo, new_node(new_rule("(\\.\\.|//)",
				  "disallow .. and //",
				  "CHGF-01",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(;&)",
				  "disallow command chaining",
				  "CHGF-02",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/(var/tmp|tmp)/protect[-._/+[:alnum:]]{0,}",
				  "wrapped functions have no access to /tmp/protect*",
				  "CHGF-03",
				  RULE_LOGALWAYS,
				  NULL)));
  return(foo);
}

list* cat_allowed()
{
  list* foo;
  foo=new_list(NULL);

  list_add(foo, new_node(new_rule(".*",
				  "allow pretty much anything so we don't break things accidentally",
				  "CAT-02",
				  0,
				  NULL)));
  if(geteuid() != 0 ){
    list_add(foo, new_node(new_rule(".*",
				    "Allow non-priv users to cat files if otherwise allowed by the system — \
effectively a passthrough for when wrappers end up in a users PATH. Exclude Rules still apply.",
				    "CAT-03",
				    0,
				    NULL)));
  }

  return(foo);
}

list* cat_forbidden()
{
  list* foo;
  foo=new_list(NULL);

  list_add(foo, new_node(new_rule("(shadow|master\\.passwd)",
				  "protect passwd info",
				  "CATF-01",
				  0,
				  NULL)));
  list_add(foo, new_node(new_rule("id_(dsa|rsa)",
				  "protect private keys",
				  "CATF-02",
				  0,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/(var/tmp|tmp)/protect[-._/+[:alnum:]]{0,}",
				  "wrapped functions have no access to /tmp/protect*",
				  "CATF-03",
				  RULE_LOGALWAYS,
				  NULL)));
  return(foo);

}


list* rmdir_allowed()
{
  list* foo;
  char* u; /* sudo user name */
  char* fee;
  foo=new_list(NULL);

  if((u=getenv("SUDO_USER")) != NULL){
    list_add(foo, new_node(new_rule((fee=my_asprintf(10240, "%s%s%s", "^(-p[[:space:]]{1,}){0,1}", sudo_user_homedir(u), "[-._/+[:alnum:]]{1,}$")),
				    "Dynamic rule to allow rmdir in real users homedir",
				    "RMDIR-01",
				    0,
				    NULL)));
    free(fee);
  }
  if(geteuid() != 0 ){
    list_add(foo, new_node(new_rule(".*",
				    "Allow non-priv users to rmdir if otherwise allowed by the system — \
effectively a passthrough for when wrappers end up in a users PATH. Exclude Rules still apply.",
				    "RMDIR-02",
				    0,
				    NULL)));
  }
  list_add(foo, new_node(new_rule("^(-p[[:space:]]{1,}){0,1}/sys/fs/cgroup(s){0,1}/[-._/+\\\\[:alnum:]]{0,}[p,P][b,B][s,S][-._/+\\\\[:alnum:]]{0,}[[:space:]]{0,}$",
				  "rmdir cgroups BZ24876, BZ25050",
				  "RMDIR-03",
				  0,
				  NULL)));
  list_add(foo, new_node(new_rule("^/dev/cpuset/[[:alnum:]]*$",
				  "rmdir for older style cpusets",
				  "RMDIR-04",
				  0,
				  NULL)));
  return(foo);
}

//size_t rm_allowed_count=PALEN(rm_allowed);

list* rmdir_forbidden()
{
  list* foo;
  foo=new_list(NULL);
  
  list_add(foo, new_node(new_rule("(\\.\\.|//)",
				  "disallow .. and //",
				  "RMDIRF-01",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(;&)",
				  "disallow command chaining",
				  "RMDIRF-02",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/var/spool(/){0,1}",
				  "protect /var/spool",
				  "RMDIRF-03",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/root/\\.[[:alnum:]]{0,}",
				  "protect /root",
				  "RMDIRF-04",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/etc/",
				  "protect /etc",
				  "RMDIRF-05",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("(?<![[:alnum:]])/(bin|sbin|usr/bin|usr/sbin|lib(64){0,1}|usr/lib(64){0,1}|libexec|usr/libexec|var/log|var/spool/|var/ossec|opt/tools)",
				  "protect system dirs",
				  "RMDIRF-06",
				  RULE_LOGALWAYS,
				  NULL)));
  list_add(foo, new_node(new_rule("/(var/tmp|tmp)/(\\.|screens|vi|ssh-|xf-dll|keyring|s\\.eventmond|ossec|protect|no-del|nodel|no_del)",
				  "protect /tmp from accidental over reach of wrapped delete",
				  "RMDIRF-07",
				  0,
				  NULL)));
  list_add(foo, new_node(new_rule("(/export/home|/home)/[-._[:alnum:]]{1,}/\\.ssh(/){0,1}",
				  "protect exploitable files in user .ssh dir from deletion",
				  "RMDIRF-08",
				  0,
				  NULL)));
  return(foo);
}
