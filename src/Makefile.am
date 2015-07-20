AM_CPPFLAGS = -I$(top_srcdir)/include

libunival_CONFIG_INCLUDES=-I$(builddir)/config
libunival_INCLUDES=-I$(builddir) -I$(builddir)/obj univalue/univalue_escapes.h ../include/univalue.h 

noinst_LIBRARIES = libunival.a libdbb.a

libunival_a_SOURCES = univalue/univalue.cpp univalue/univalue_read.cpp univalue/univalue_write.cpp

libdbb_a_INCLUDES = ../include/dbb.h libdbb/util.h libdbb/crypto.h
libdbb_a_SOURCES = libdbb/dbb.cpp libdbb/base64.cpp libdbb/crypto.cpp libdbb/util.h

dbb_cli_CONFIG_INCLUDES=-I$(builddir)/config
dbb_cli_INCLUDES=-I$(builddir) -I$(builddir)/obj

bin_PROGRAMS = dbb-cli

dbb_cli_SOURCES = dbb_cli.cpp util.h util.cpp
dbb_cli_CPPFLAGS = $(AM_CPPFLAGS) 
dbb_cli_CFLAGS = 
dbb_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)
dbb_cli_LDADD = libunival.a libdbb.a $(CRYPTO_LIBS)


bin_PROGRAMS += dbb-daemon

qt/ui/ui_%.h: qt/ui/%.ui
	$(UIC) -o $@ $< || (echo "Error creating $@"; false)

qt/qrc_%.cpp: qt/%.qrc
	$(RCC) -o $@ $< || (echo "Error creating $@"; false)

qt/moc_%.cpp: qt/%.h
	$(MOC) -o $@ $< || (echo "Error creating $@"; false)
	
dbb_daemon_SOURCES = dbb_daemon.cpp util.h util.cpp 
dbb_daemon_SOURCES += qt/daemongui.h qt/daemongui.cpp qt/moc_daemongui.cpp qt/ui/ui_daemon.h qt/qrc_daemon.cpp
dbb_daemon_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) $(QT_DBUS_INCLUDES)
dbb_daemon_CFLAGS = 
dbb_daemon_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(QT_LDFLAGS) $(LIBEVENT_LDFLAGS)
dbb_daemon_LDADD = libunival.a libdbb.a $(LIBEVENT_LIBS) $(CRYPTO_LIBS) $(QT_LIBS) $(QT_DBUS_LIBS)
