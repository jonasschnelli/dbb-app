dnl require autoconf 2.60 (AS_ECHO/AS_ECHO_N)
AC_PREREQ([2.60])
AC_INIT([Digital Bitbox CLI],[0.1],[https://github.com/jonasschnelli/dbb_cli/issues],[dbb_cli])
AC_CONFIG_SRCDIR([src/config])
AC_CONFIG_HEADERS([src/config/dbb_cli-config.h])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([no-define subdir-objects foreign])

AC_PROG_RANLIB

# Enable debug
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [use debug compiler flags and macros (default is no)])],
    [enable_debug=$enableval],
    [enable_debug=no])

if test "x$enable_debug" = xyes; then
    CFLAGS="-g3 -O0 -DDEBUG"
    CXXFLAGS="-g3 -O0 -DDEBUG"
		AC_DEFINE_UNQUOTED([ENABLE_DEBUG],[1],[Define to 1 to enable debung output])
fi

AC_ARG_WITH(hid-libdir,
[  --with-hid-libdir=DIR        specify exact library dir for hid library
  --without-hid        disables hid usage completely],
hid_libdir="$withval")

AC_ARG_WITH(hid-includedir,
[  --with-hid-includedir=DIR        specify exact includes dir for hid library],
hid_includedir="$withval")

if test "${with_hid}" != no ; then
	OLD_LIBS=$LIBS
	OLD_LDFLAGS=$LDFLAGS
	OLD_CFLAGS=$CFLAGS
	OLD_CPPFLAGS=$CPPFLAGS
	if test "${hid_libdir}" ; then
		LDFLAGS="$LDFLAGS -L${hid_libdir}"
		AC_MSG_NOTICE([found $hid_libdir])
	fi
	if test "${hid_includedir}" ; then
		CFLAGS="$CFLAGS -I${hid_includedir}"
		CPPFLAGS="$CPPFLAGS -I${hid_includedir}"
	fi
	no_good=no
	AC_CHECK_LIB([hidapi],[hid_open],,no_good=yes)
	AC_CHECK_HEADER([hidapi/hidapi.h],,no_good=yes)
	if test "$no_good" = yes; then
		AC_MSG_ERROR([*** Please install libhidapi on your system ***])
	fi
fi

AC_CHECK_LIB([crypto],[main],,[AC_MSG_ERROR([*** Please install openssl on your system ***])])
LDFLAGS="$LDFLAGS -Lcrypto"
AC_PROG_CXX
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
