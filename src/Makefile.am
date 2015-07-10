dbb_cli_CONFIG_INCLUDES=-I$(builddir)/config
dbb_cli_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BOOST_CPPFLAGS)

bin_PROGRAMS = dbb_cli

dbb_cli_SOURCES = dbb_cli.cpp
dbb_cli_CPPFLAGS = $(GITIAN_VERIFIER_INCLUDES)
dbb_cli_CFLAGS = 
dbb_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

dbb_cli_LDADD = 
