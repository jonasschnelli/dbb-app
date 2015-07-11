libunival_CONFIG_INCLUDES=-I$(builddir)/config
libunival_INCLUDES=-I$(builddir) -I$(builddir)/obj univalue/univalue_escapes.h univalue/univalue.h 

noinst_LIBRARIES = libunival.a

libunival_a_SOURCES = univalue/univalue.cpp univalue/univalue_read.cpp univalue/univalue_write.cpp
libunival_a_CPPFLAGS = 
libunival_a_CFLAGS = 


dbb_cli_CONFIG_INCLUDES=-I$(builddir)/config
dbb_cli_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BOOST_CPPFLAGS)

bin_PROGRAMS = dbb_cli

dbb_cli_INCLUDES = util.h crypto.h
dbb_cli_SOURCES = dbb_cli.cpp base64.cpp crypto.cpp util.h util.cpp
dbb_cli_CPPFLAGS = 
dbb_cli_CFLAGS = 
dbb_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

dbb_cli_LDADD = libunival.a
