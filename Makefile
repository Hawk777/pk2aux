world: apps

CFLAGS := -Wall -Wextra -O2 -march=native -iquote lib/include `pkg-config --cflags libusb-1.0`
LIBS := `pkg-config --libs libusb-1.0`
APPS := id ls pin reset uart ver

# Include the library makefile and each app's makefile.
include lib/Makefile.inc
include ${APPS:%=%/Makefile.inc}

# "apps" builds all apps, each to a binary named "pk2%" where %=appname
apps: ${APPS:%=pk2%}

# Each app named "pk2%" depends on its object modules, the library, libusb, and libm.
# Also, each of the app's object files depends on the public header file.
define APP_TEMPLATE
pk2$(1): $$($(1)_OBJS) lib/libpk2aux.a -lm
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LIBS) -opk2$(1) $$+

$$($(1)_OBJS): lib/include/pk2aux.h
endef
${foreach APP,${APPS},${eval ${call APP_TEMPLATE,${APP}}}}

# Clean by removing all app binaries and app object modules, plus cleaning the library.
clean: clean-lib
	-rm -f ${APPS:%=pk2%}
	-rm -f ${foreach APP,${APPS},${${APP}_OBJS}}

# Install the apps by running GNU Install and putting them in /usr/local/bin/.
install: apps
	install -m0755 ${APPS:%=pk2%} /usr/local/bin/

# Build documentation by running Doxygen.
doc:
	doxygen

.PHONY: world apps clean install doc

