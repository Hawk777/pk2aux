world: apps

CFLAGS := -Wall -Wextra -g -iquote lib/include
APPS := ls reset id pin uart

# Include the library makefile and each app's makefile.
include lib/Makefile.inc
include ${APPS:%=%/Makefile.inc}

# "apps" builds all apps, each to a binary named "pk2%" where %=appname
apps: ${APPS:%=pk2%}

# Each app named "pk2%" depends on its object modules, the library, libusb, and libm.
# Also, each of the app's object files depends on the public header file.
define APP_TEMPLATE
pk2$(1): $$($(1)_OBJS) lib/libpk2aux.a -lusb -lm
	$(CC) $(CFLAGS) $(CPPFLAGS) -opk2$(1) $$+

$$($(1)_OBJS): lib/include/pk2aux.h
endef
${foreach APP,${APPS},${eval ${call APP_TEMPLATE,${APP}}}}

# Clean by removing all app binaries and app object modules, plus cleaning the library.
clean: clean-lib
	-rm -f ${APPS:%=pk2%}
	-rm -f ${foreach APP,${APPS},${${APP}_OBJS}}

.PHONY: world apps clean

