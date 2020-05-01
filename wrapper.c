/* 
Copyright 2020 Caravan Electronic Publishing, LLC

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


*/

#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "wrapper.h"
#include "ruledefs.h"
#include "list.h"

/* on old 32bit linux sometimes st_mode flags don't get defined */
#if !defined S_IFREG
#   define S_IFREG 0100000
#   define S_IFDIR 0040000
#   define S_IFLNK 0120000
#   define S_ISUID 0004000
#   define S_ISGID 0002000
#endif

#ifndef LOG_AUTHPRIV
#define LOG_AUTHPRIV LOG_AUTH
#endif


/* global for holding the name the wrapper executable was called as */
char myname[PATH_MAX + 1];
char syslog_id[PATH_MAX + 1 + 9]; /* wrapper(myname) */

void usage()
{
  fprintf(stderr, "usage: %s version %s file1 ... [filen]\n", myname, VERSION);
  exit(99);
}

void run_unwrapped(char* argv[])
{
  int ret;

#ifdef TESTONLY
  exit(0);
#endif
  
  if(strcmp(myname, "rm") == 0){
    argv[0] = "/bin/rm";
  }else if(strcmp(myname, "cp") == 0){
    argv[0] = "/bin/cp";
  }else if(strcmp(myname, "mkdir") == 0){
    argv[0] = "/bin/mkdir";
  }else if(strcmp(myname, "chmod") == 0){
    argv[0] = "/bin/chmod";
  }else if(strcmp(myname, "chown") == 0){
    argv[0] = "/bin/chown";
  }else if(strcmp(myname, "chgrp") == 0){
    argv[0] = "/bin/chgrp";
  }else if(strcmp(myname, "cat") == 0){
    argv[0] = "/bin/cat";
  }else if(strcmp(myname, "rmdir") == 0){
    argv[0] = "/bin/rmdir";
  }
  
  ret = execv(argv[0], argv);
  exit(ret);
  
}

/*
  1. determine which command we are wrapping based on argv[0]'s basename
  2. check that it MATCHES an allowed command. MATCH stops with first rule matched,
     so more specific rules should come before more general ones, and rules 
     with RULE_FINAL set should always come first.
  3. IF allowed, MATCH command line to FORBIDDEN regular expressions. If RULE_FINAL
     was matched for the allowed rules, then we may log an error message via syslog
     if RULE_LOGALWAYS is set, but execution below will still take place.
  4. finally, EXEC the unwrapped executable with the command line we were passed.
 */

int main(int argc, char** argv)
{

  char* real_user;
  int i, l;
  char* c;
  char* cl;
  char mycwd[PATH_MAX];
  size_t mycwd_len, cl_len=0, copied=0;
  regex_rule* matched;
  list* allowed;
  list* forbidden;
#ifdef RULEDUMP
  node* n;
#endif
  struct stat fileinfo;
  int exists=0;

  if((real_user = getenv("SUDO_USER")) == NULL ){
    /* real_user="root"; */
  }
  
  strncpy(myname, (c = strrchr(argv[0],'/')) == NULL ? argv[0] : &c[1], PATH_MAX);
  myname[PATH_MAX] = '\0'; /* argv[0] can't be more than PATH_MAX, but paranoia is ok */

  /* 
     basic argument checking
  */
  
  if( argc == 2 && argv[1][0] == '-' &&
	    ( strcmp(argv[1], "--version") == 0 ) ){
    printf("wrappers %s\n", VERSION);
    exit(0);
  }

  /* we don't wrap if not running as root */
#ifndef TESTONLY
  if(geteuid() != (uid_t)0){
    run_unwrapped(argv);
  }
  
  /* open syslog */

  sprintf(syslog_id, "wrapper(v. %s)(%s)[%s:%u]", VERSION, myname, real_user == NULL ? "-" : real_user, geteuid() );
  openlog(syslog_id, LOG_CONS, LOG_AUTHPRIV);
#endif
  /* 
     Calculate buffer needed to make one big command line string.
     not wide char safe! 

     Then malloc buffer and copy entire command line into it.
  */


  /* get our cwd in case we need to make an argument non-relative */
  getcwd(mycwd, PATH_MAX);
  strcat(mycwd, "/");
  mycwd_len = strlen(mycwd);
  
  for(i = 1; i < argc; ++i){
    if( argv[i][0] == '/' ||  argv[i][0] == '-' ){
      cl_len += strlen(argv[i]);
    }else{ /* could be up to PATH_MAX once we make it absolute */
      cl_len += mycwd_len + strlen(argv[i]);
    }
  }
  cl_len += (argc - 1); /* arguments sans argv[0], with sep or \0 for each */

  /* build the new command line and copy it into the new buffer of size
     cl_len */
  
  if(cl_len == 0){ /* without arguments, e.g. when wrapping "cat" */
    cl = malloc(sizeof(char) * 1);
    cl="\0";
  }else{ 
    cl = malloc(cl_len);
  }
  
  for(i = 1; i < argc; ++i){

    /* see if file exists, and if so, stat its inode info */
    if(stat(argv[i], &fileinfo) == 0){
      exists=1;
    }else{
      exists=0;
    }
      
    /* See if argument is a relative file or dir and if so, make it non-relative. */
    if(exists == 1 && argv[i][0] != '/' && argv[i][0] != '-'){
      if(( fileinfo.st_mode & (S_IFDIR|S_IFREG|S_IFLNK)) != 0){
	/* write cwd to cl buffer now before we copy the argument */
	memcpy(&cl[copied], mycwd, mycwd_len);
	copied += mycwd_len;
      }else if(strcmp(myname, "rm") == 0) { /* rm -f should work for files that don't exist */
	memcpy(&cl[copied], mycwd, mycwd_len);
	copied += mycwd_len;
      }
    }
    /* copy the argument to the cl and append a space */
    memcpy(&cl[copied], argv[i], (l=strlen(argv[i])));
    copied += l;
    cl[copied++]=' ';

    /* while in this loop, it's also a good time to look at some special cases,
       e.g. the case of "chown root" and setuid bits */

    if((i == 1) && ((strcmp(myname, "chown") == 0) || (strcmp(myname, "chgrp") == 0) || (strcmp(myname, "chmod") == 0))){
      /* This is a special case we can ignore. The first argument to chown, chgrp or chmod is never a file or directory. */
      continue;
    }
    
    if( (strcmp(myname, "chown") == 0) && (exists == 1 ) && ((fileinfo.st_mode & (S_ISUID)) != 0)){
#ifndef TESTONLY      
#if !defined(__linux__)
      syslog(LOG_WARNING, "\'%s %s\' security error, chown of a setuid file was attempted, silently clearing setuid bit to mimic common linux behavior on non-linux platform", myname, cl);
      chmod(argv[i], fileinfo.st_mode ^ ( S_ISUID ));
      /* fprintf(stderr, "chown of a setuid file is a security violation. chmod must come after chown and be allowed by wrapper rules\n"); 
	 exit(99); */
#else
      syslog(LOG_WARNING, "\'%s %s\' security error, chown of a setuid file was attempted, chown on linux will silently clear the setuid bit", myname, cl); 
#endif
#endif     
    }
    
    if( (strcmp(myname, "chgrp") == 0) && (exists == 1 ) && ((fileinfo.st_mode & (S_ISGID)) != 0)){
#ifndef TESTONLY      
#if !defined(__linux__)
      syslog(LOG_WARNING, "\'%s %s\' security error, chgrp of a setgid file was attempted, silently clearing setgid bit to mimic common linux behavior on non-linux platform", myname, cl);
      chmod(argv[i], fileinfo.st_mode ^ ( S_ISGID ));
      /* fprintf(stderr, "chgrp of a setgid file is a security violation. chmod must come after chgrp and be allowed by wrapper rules\n"); 
	 exit(99); */
#else
      syslog(LOG_WARNING, "\'%s %s\' security error, chgrp of a setuid file was attempted, chgrp on linux will silently clear the setgid bit", myname, cl); 
#endif
#endif     
    }
  }
  if( copied > 0 ){
    cl[copied - 1]='\0'; /* terminate, terminate! */
  }

  /* 
     since we may be in the path of the user, whether sudo is used or not, 
     we should not perform checks ( other then chown + setuid, etc... above)
     if we get call normally, but instead should just fall
     through and let system permissions do their job 
  */
#ifndef TESTONLY
  if( real_user == NULL ){
    if( geteuid() == 0 ){
      /*      
	      syslog(LOG_ERR, "\'%s %s\' invoked by root without using sudo", myname, cl);
      */
    }else{
      run_unwrapped(argv); /* a non-root user probably used us accidentally */
    }
  }
#endif
  
  /* using myname, apply appropriate regex to cl */
  
  if(strcmp(myname, "rm") == 0){
    allowed=rm_allowed();
    forbidden=rm_forbidden();    
  }else if(strcmp(myname, "cp") == 0){
    allowed=cp_allowed();
    forbidden=cp_forbidden();
  }else if(strcmp(myname, "mkdir") == 0){
    allowed=mkdir_allowed();
    forbidden=mkdir_forbidden();
  }else if(strcmp(myname, "chmod") == 0){
    allowed=chmod_allowed();
    forbidden=chmod_forbidden();
  }else if(strcmp(myname, "chown") == 0){
    allowed=chown_allowed();
    forbidden=chown_forbidden();
  }else if(strcmp(myname, "chgrp") == 0){
    allowed=chgrp_allowed();
    forbidden=chgrp_forbidden();
  }else if(strcmp(myname, "cat") == 0){
    /* special case for cat without an argument */
    if( argc == 1){
      run_unwrapped(argv);
    }
    allowed=cat_allowed();
    forbidden=cat_forbidden();
  }else if(strcmp(myname, "rmdir") == 0){
      allowed=rmdir_allowed();
      forbidden=rmdir_forbidden();
  }else{
#ifndef TESTONLY
    syslog(LOG_WARNING, "no wrapper for %s", myname);
#endif
    fprintf(stderr, "I don't know how to wrap %s\n", myname);
    closelog();
    exit(99);
  }


#ifdef RULEDUMP
  printf("dumping all rules for %s\n", myname);
  printf("Allowed rules:\n");
  for(n=list_head(allowed); n != NULL; n=list_next(allowed)){
    printf("%s:'%s'\n", ((regex_rule *)(n->data))->id, ((regex_rule *)(n->data))->subject );
  }
  printf("\n");
  printf("Forbidden rules:\n");
  for(n=list_head(forbidden); n != NULL; n=list_next(forbidden)){
    printf("%s:'%s'\n", ((regex_rule *)(n->data))->id, ((regex_rule *)(n->data))->subject );
  }
  printf("\n");
#endif


#ifdef TESTONLY
  printf("adjusted command args are: %s\n", cl);
  if( (matched=apply_rules(allowed, cl)) != NULL ){
    printf("matched allowed %s rule id: %s\n",  (matched->flags & RULE_FINAL) ? "FINAL":"", matched->id);
  }else{
    printf("no allowed rule matched\n");    
  }
  if( (matched=apply_rules(forbidden, cl)) != NULL ){
    printf("matched forbidden rule id: %s\n", matched->id);
  }
  exit(0);
#endif
  
  
  if( (matched=apply_rules(allowed, cl)) != NULL ){
    if(matched->flags & RULE_FINAL){
      /* check against forbidden, e.g. to get logging, but ignore the result */
      apply_rules(forbidden, cl);
    }else
      if( (matched=apply_rules(forbidden, cl)) != NULL){
	if(!(matched->flags & RULE_LOGALWAYS)){
	  /* don't syslog if apply rules already did it */
	  syslog(LOG_ERR, "%s forbidden by rule %s", cl, matched->id);
	}
	fprintf(stderr, "command %s %s violated rule %s\n", myname, cl, matched->id);
	closelog();
	exit(99);
      }
  }else{
    syslog(LOG_ERR, "no match for \'%s\'", cl);
    closelog();
    fprintf(stderr, "no allowed rule matched\n");
    exit(99);
  }
  
  /*  printf("running unwrapped command %s\n", cl ); */
  run_unwrapped(argv);
  
  return(99); /* we don't actually reach this as we call exit in run_unwrapped*/
}




		   
