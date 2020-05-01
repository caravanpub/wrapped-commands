A system for creating wrapped versions of command line executables.
Each wrapped executable is evaluated using a set of allowed and
forbidden regular expression matches. 

Command execution is logged. It's posssible to add comments to
rules which tie the execution of a command to a policy.

This is something I created for my customers several years ago
for use where sudo is insufficient and where speed is also
paramount. Scripts, programs that read external files or databases,
etc... are all too slow for wrapping a command like rm which may
be run many thousands of times a day. 

The PCRE expressions are compiled into the wrapper for speed. This does
mean that these executables do need to be copied to the systems
on which they are run or placed on a file server.

To install:

1. create the policy rules in ruledefs.c. cp, mkdir, rm, and a
   few other commands are wrapped by default. You will need to update
   ruledefs.h and wrapper.c if you wish to add additional commands
   and their rule lists.

2. compile with make. ( gmake ) This has been run on linux, solaris, AIX,
   HPUX, and a few BSD variants. Make changes to the MAKEFILE
   as needed to support other platforms.

3. Copy the files to some location. I use /opt/tools/wrappers.

4. If you want these to run as root, update the sudoers
   file to allow these to be run by the users
   who need them without argument checking — if such checking was possible,
   you wouldn't need these wrapped commands.

   Cmnd_Alias  WRAPPED = /opt/tools/wrappers/rm, \
   	                 /opt/tools/wrappers/cp, \
                         /opt/tools/wrappers/mkdir, \
                         /opt/tools/wrappers/chmod, \
                         /opt/tools/wrappers/cat
		      
   or just /opt/tools/wrapped/* if you're less paranoid than I am. 

5. The accounts will need to updated PATHs to place these before the
   unwrapped commands. ( up to you if this happens in their shell config or
   if it's only done in scripts which need sudo )