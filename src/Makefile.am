libunival_CONFIG_INCLUDES=-I$(builddir)/config
libunival_INCLUDES=-I$(builddir) -I$(builddir)/obj univalue/univalue_escapes.h ../include/univalue.h 

noinst_LIBRARIES = libunival.a libdbb.a

libunival_a_SOURCES = univalue/univalue.cpp univalue/univalue_read.cpp univalue/univalue_write.cpp

libdbb_a_INCLUDES = ../include/dbb.h libdbb/util.h libdbb/crypto.h
libdbb_a_SOURCES = libdbb/dbb.cpp libdbb/base64.cpp libdbb/crypto.cpp libdbb/util.h

dbb_cli_CONFIG_INCLUDES=-I$(builddir)/config
dbb_cli_INCLUDES=-I$(builddir) -I$(builddir)/obj

bin_PROGRAMS = dbb_cli

dbb_cli_SOURCES = dbb_cli.cpp util.h util.cpp
dbb_cli_CPPFLAGS = 
dbb_cli_CFLAGS = 
dbb_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)
dbb_cli_LDADD = libunival.a libdbb.a
