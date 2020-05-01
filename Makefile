VERSION=20200501p0
TARGETS=wrapper
WRAPPEROBJS=wrapper.o regex.o ruledefs.o my_asprintf.o list.o rule.o
WRAPPED=rm cp mkdir chmod chown chgrp cat rmdir
DESTROOT=/opt/tools

#since this is developed on OSX, some accomodation is required
UNAME := $(shell uname -s)

#defaults to be overridden
CFLAGS ?= -g -I. -I$(DESTROOT)/include -I/usr/pkg/include -I/opt/local/include -I/usr/local/include
LDFLAGS ?= -L$(DESTROOT)/lib -L/usr/local/lib

#This has not been tested on OS X, but this allows me to
#build and run these commands in simulation mode,
#which is absolutely critical for working with complex rules.
#
#I know this is a hack and I'll improve this later.
# "The perfect is the enemy of the good"
ifeq ($(UNAME), Darwin)
CFLAGS = -g -I. -I/opt/local/include -DRULEDUMP -DTESTONLY 
LDFLAGS = -L/opt/local/lib
endif


ifeq ($(UNAME), HP-UX)
CFLAGS = -I. -I$(DESTROOT)/include
LDFLAGS = -shared-libgcc -L$(DESTROOT)/lib
endif

ifeq ($(UNAME), NetBSD)
MAKE=gmake
#we really only debug on netbsd, so add -g
CFLAGS= -g -I. -I$(DESTROOT)/include -I/usr/pkg/include
LDFLAGS = -L$(DESTROOT)/lib -L/usr/pkg/lib -R$(DESTROOT)/lib -R/usr/pkg/lib
endif

.DUMMY:	all

all: $(TARGETS) 

#with use of macros, it's useful to have the preprocessed rules available
ifeq ($(UNAME), Darwin)
ruledefs.i: ruledefs.c
	$(CC) -E -o ruledefs.i ruledefs.c
endif

wrapper: $(WRAPPEROBJS)
	$(CC) -o wrapper $(WRAPPEROBJS) $(LDFLAGS) -lpcre
	for p in $(WRAPPED);do /bin/cp -p wrapper $$p;done

install:
	if test ! -d $(DESTROOT)/wrappers;then /bin/mkdir -p $(DESTROOT)/wrappers;fi
	for p in $(WRAPPED);do if test -f $(DESTROOT)/wrappers/$$p;then /bin/mv $(DESTROOT)/wrappers/$$p $(DESTROOT)/wrappers/$$p.old;fi ;/bin/cp -p $$p $(DESTROOT)/wrappers;done


clean:
	rm -rf $(WRAPPEROBJS) wrapper ${WRAPPED} wrappers*.tar.gz wrappers-${VERSION} *.i

distclean:
	rm -rf *.o *~ wrapper wrap_runner wrap_validate ${WRAPPED} *.tgz wrappers-${VERSION} wrappers*.tar.gz *.i

release:
	rm -rf wrappers-${VERSION} wrapper*.tar.gz
	mkdir wrappers-${VERSION}
	cp -rp sha2 *.c *.h Makefile COPYRIGHT wrappers-${VERSION}
	tar -cf - wrappers-${VERSION} | gzip > wrappers-${VERSION}.tar.gz
	rm -rf wrappers-${VERSION}

# DO NOT DELETE

list.o: list.h
regex.o: list.h wrapper.h rule.h regex.h version.h
rule.o: rule.h
ruledefs.o: my_asprintf.h wrapper.h rule.h regex.h list.h version.h
wrapper.o: wrapper.h rule.h regex.h list.h version.h ruledefs.h
