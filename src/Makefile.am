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


#check if we should build the daemon
if ENABLE_DAEMON
bin_PROGRAMS += dbb-daemon

dbb_daemon_SOURCES = dbb_daemon.cpp util.h util.cpp
dbb_daemon_CPPFLAGS = -fPIC $(AM_CPPFLAGS)
dbb_daemon_CFLAGS =
dbb_daemon_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(LIBEVENT_LDFLAGS)
dbb_daemon_LDADD = libdbb.a $(LIBEVENT_LIBS) $(CRYPTO_LIBS) $(LINUX_LIBS) libunival.a


if ENABLE_QT

QT_FORMS_UI = \
  qt/ui/daemon.ui \
	qt/ui/transactiondialog.ui

QT_MOC_CPP = \
	qt/moc_daemongui.cpp

QT_MOC = \
  qt/daemongui.moc

QT_FORMS_H=$(join $(dir $(QT_FORMS_UI)),$(addprefix ui_, $(notdir $(QT_FORMS_UI:.ui=.h))))

$(QT_MOC): $(QT_FORMS_H)
$(dbb_daemon_OBJECTS) $(dbb_daemon_OBJECTS) : | $(QT_MOC)

dbb_daemon_SOURCES += qt/daemongui.h qt/daemongui.cpp qt/qrc_daemon.cpp $(QT_MOC_CPP) $(QT_MOC) $(QT_FORMS_UI)
dbb_daemon_CPPFLAGS += $(QT_INCLUDES) $(QT_DBUS_INCLUDES)
dbb_daemon_LDFLAGS += $(QT_LDFLAGS)
dbb_daemon_LDADD += $(QT_LIBS) $(QT_DBUS_LIBS)

CLEAN_QT = $(QT_MOC_CPP) $(QT_FORMS_H)

CLEANFILES = $(CLEAN_QT)

dbb_daemon_clean: FORCE
	rm -f $(CLEAN_QT) dbb-daemon$(EXEEXT)

ui_%.h: %.ui
	@test -f $(UIC)
	@$(MKDIR_P) $(@D)
	$(UIC) -o $@ $< || (echo "Error creating $@"; false)

qt/qrc_%.cpp: qt/%.qrc
	$(RCC) -o $@ $< || (echo "Error creating $@"; false)

qt/moc_%.cpp: qt/%.h
	$(MOC) -o $@ $< || (echo "Error creating $@"; false)

endif

endif
