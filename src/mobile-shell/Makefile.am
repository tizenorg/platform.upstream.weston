lib_LTLIBRARIES = libmobile-shell-client.la

pkginclude_HEADERS =               \
	input-method-client-protocol.h		\
	mobile-shell-client-protocol.h

libmobile_shell_client_la_LIBADD = -lrt -lm
libmobile_shell_client_la_LDFLAGS = -version-info 0:9:0
libmobile_shell_client_la_SOURCES =          \
	input-method-protocol.c			\
	mobile-shell-protocol.c		

EXT_CFLAGS =

git-version.h : .FORCE
	$(AM_V_GEN)(echo "#define BUILD_ID \"$(shell git --work-tree=$(top_srcdir) describe --always --dirty) $(shell git --work-tree=$(top_srcdir) log -1 --format='%s (%ci)')\"" > $@-new; \
	cmp -s $@ $@-new || cp $@-new $@; \
	rm $@-new)

.FORCE :

moduledir = @libdir@/weston
module_LTLIBRARIES = $(mobile_shell)

# Mobile-Shell
mobile_shell = mobile-shell.la
mobile_shell_la_LDFLAGS = -module -avoid-version
mobile_shell_la_LIBADD = $(MOBILE_SHELL_LIBS)
mobile_shell_la_CFLAGS = $(GCC_CFLAGS) $(EXT_CFLAGS) $(MOBILE_SHELL_CFLAGS)
mobile_shell_la_SOURCES =			\
mobile_shell_la_SOURCES =			\
	mobile-shell.c					\
	vconf.c					\
	input-method-protocol.c			\
	input-method-server-protocol.h	

# Wayland protocol(Build by wayland-scanner)
BUILT_SOURCES =						\
	input-method-protocol.c			\
	input-method-server-protocol.h	\
	input-method-client-protocol.h	\
	mobile-shell-protocol.c \
	mobile-shell-server-protocol.h

CLEANFILES = $(BUILT_SOURCES)

wayland_protocoldir = $(top_srcdir)/src/mobile-shell/protocol
include $(top_srcdir)/wayland-scanner.mk
