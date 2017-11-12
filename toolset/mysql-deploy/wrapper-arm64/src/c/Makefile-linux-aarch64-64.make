# Copyright (c) 1999, 2017 Tanuki Software, Ltd.
# http://www.tanukisoftware.com
# All rights reserved.
#
# This software is the proprietary information of Tanuki Software.
# You shall use it only in accordance with the terms of the
# license agreement you entered into with Tanuki Software.
# http://wrapper.tanukisoftware.com/doc/english/licenseOverview.html

COMPILE = gcc -O3 -fPIC -Wall --pedantic  -DLINUX -DJSW64 -D_FILE_OFFSET_BITS=64 -fpic -D_GNU_SOURCE -DUNICODE -D_UNICODE

INCLUDE=$(JAVA_HOME)/include

DEFS = -I$(INCLUDE) -I$(INCLUDE)/linux

wrapper_SOURCE = wrapper.c wrapperinfo.c wrappereventloop.c wrapper_unix.c property.c logger.c logger_file.c wrapper_file.c wrapper_i18n.c wrapper_hashmap.c wrapper_ulimit.c

libwrapper_so_OBJECTS = wrapper_i18n.o wrapperjni_unix.o wrapperinfo.o wrapperjni.o loggerjni.o

testsuite_SOURCE = testsuite.c test_example.c test_javaadditionalparam.c test_hashmap.c test_filter.c wrapper.c wrapperinfo.c wrappereventloop.c wrapper_unix.c property.c logger.c logger_file.c wrapper_file.c wrapper_i18n.c wrapper_hashmap.c wrapper_ulimit.c

BIN = ../../bin
LIB = ../../lib
TEST = ../../test

all: init wrapper libwrapper.so testsuite

clean:
	rm -f *.o

cleanall: clean
	rm -rf *~ .deps
	rm -f $(BIN)/wrapper $(LIB)/libwrapper.so

init:
	if test ! -d .deps; then mkdir .deps; fi

wrapper: $(wrapper_SOURCE)
	$(COMPILE) -pthread $(wrapper_SOURCE) -lm -o $(BIN)/wrapper

testsuite: $(testsuite_SOURCE)
	$(COMPILE) -DCUNIT $(testsuite_SOURCE) -lm -pthread -L/usr/local/lib -lncurses -lcunit -o $(TEST)/testsuite

libwrapper.so: $(libwrapper_so_OBJECTS)
	${COMPILE} -shared $(libwrapper_so_OBJECTS) -o $(LIB)/libwrapper.so

%.o: %.c 
	@echo '$(COMPILE) -c $<'; \
	$(COMPILE) $(DEFS) -Wp,-MD,.deps/$(*F).pp -c $<
	@-cp .deps/$(*F).pp .deps/$(*F).P; \
	tr ' ' '\012' < .deps/$(*F).pp \
	| sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
	>> .deps/$(*F).P; \
	rm .deps/$(*F).pp
