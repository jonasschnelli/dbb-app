dnl require autoconf 2.60 (AS_ECHO/AS_ECHO_N)
AC_PREREQ([2.60])
AC_INIT([Digital Bitbox CLI],[0.1],[https://github.com/jonasschnelli/dbb_cli/issues],[dbb_cli])
AC_CONFIG_SRCDIR([src/config])
AC_CONFIG_HEADERS([src/config/dbb-config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([no-define subdir-objects foreign])

AX_CXX_COMPILE_STDCXX_11()

LT_INIT([pic-only])

# Enable debug
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [use debug compiler flags and macros (default is no)])],
    [enable_debug=$enableval],
    [enable_debug=no])

if test "x$enable_debug" = xyes; then
    CFLAGS="$CFLAGS -g3 -O0 -DDEBUG"
    CXXFLAGS="$CXXFLAGS -g3 -O0 -DDEBUG"
		AC_DEFINE_UNQUOTED([ENABLE_DEBUG],[1],[Define to 1 to enable debung output])
fi

AC_ARG_ENABLE([daemon],
    [AS_HELP_STRING([--enable-daemon],
  								[enable daemon (default is no)])],
    [enable_daemon=$enableval],
    [enable_daemon=yes])

if test x$enable_daemon] != xno; then
	lldp_CHECK_LIBEVENT
  AC_DEFINE_UNQUOTED([ENABLE_DAEMON],[1],[Define to 1 to enable the dbb daemon])
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
	AC_SEARCH_LIBS([hid_open],[hidapi hidapi-hidraw],,no_good=yes)
	AC_CHECK_HEADER([hidapi/hidapi.h],,no_good=yes)
	if test "$no_good" = yes; then
		AC_MSG_ERROR([*** Please install libhidapi on your system ***])
	fi
fi







AX_CHECK_OPENSSL

use_pkgconfig=yes



case $host in
  *mingw*)

     #pkgconfig does more harm than good with MinGW
     use_pkgconfig=no

     TARGET_OS=windows
     AC_CHECK_LIB([mingwthrd],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([kernel32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([user32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([gdi32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([comdlg32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([winspool],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([winmm],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([shell32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([comctl32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([ole32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([oleaut32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([uuid],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([rpcrt4],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([advapi32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([ws2_32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([mswsock],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([shlwapi],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([iphlpapi],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([crypt32],      [main],, AC_MSG_ERROR(lib missing))

     # -static is interpreted by libtool, where it has a different meaning.
     # In libtool-speak, it's -all-static.
     AX_CHECK_LINK_FLAG([[-static]],[LIBTOOL_APP_LDFLAGS="$LIBTOOL_APP_LDFLAGS -all-static"])

     AC_PATH_PROG([MAKENSIS], [makensis], none)
     if test x$MAKENSIS = xnone; then
       AC_MSG_WARN("makensis not found. Cannot create installer.")
     fi

     AC_PATH_TOOL(WINDRES, windres, none)
     if test x$WINDRES = xnone; then
       AC_MSG_ERROR("windres not found")
     fi

     CPPFLAGS="$CPPFLAGS -D_MT -DWIN32 -D_WINDOWS -DBOOST_THREAD_USE_LIB"
     LEVELDB_TARGET_FLAGS="TARGET_OS=OS_WINDOWS_CROSSCOMPILE"
     if test "x$CXXFLAGS_overridden" = "xno"; then
       CXXFLAGS="$CXXFLAGS -w"
     fi
     case $host in
       i?86-*) WINDOWS_BITS=32 ;;
       x86_64-*) WINDOWS_BITS=64 ;;
       *) AC_MSG_ERROR("Could not determine win32/win64 for installer") ;;
     esac
     AC_SUBST(WINDOWS_BITS)

     dnl libtool insists upon adding -nostdlib and a list of objects/libs to link against.
     dnl That breaks our ability to build dll's with static libgcc/libstdc++/libssp. Override
     dnl its command here, with the predeps/postdeps removed, and -static inserted. Postdeps are
     dnl also overridden to prevent their insertion later.
     dnl This should only affect dll's.
     archive_cmds_CXX="\$CC -shared \$libobjs \$deplibs \$compiler_flags -static -o \$output_objdir/\$soname \${wl}--enable-auto-image-base -Xlinker --out-implib -Xlinker \$lib"
     postdeps_CXX=

     ;;
  *darwin*)
     TARGET_OS=darwin
     LEVELDB_TARGET_FLAGS="TARGET_OS=Darwin"
     if  test x$cross_compiling != xyes; then
       BUILD_OS=darwin
       AC_CHECK_PROG([PORT],port, port)
       if test x$PORT = xport; then
         dnl add default macports paths
         CPPFLAGS="$CPPFLAGS -isystem /opt/local/include"
         LIBS="$LIBS -L/opt/local/lib"
         if test -d /opt/local/include/db48; then
           CPPFLAGS="$CPPFLAGS -I/opt/local/include/db48"
           LIBS="$LIBS -L/opt/local/lib/db48"
         fi
       fi

       AC_CHECK_PROG([BREW],brew, brew)
       if test x$BREW = xbrew; then
         dnl These Homebrew packages may be keg-only, meaning that they won't be found
         dnl in expected paths because they may conflict with system files. Ask
         dnl Homebrew where each one is located, then adjust paths accordingly.
         dnl It's safe to add these paths even if the functionality is disabled by
         dnl the user (--without-wallet or --without-gui for example).

         openssl_prefix=`$BREW --prefix openssl 2>/dev/null`
         qt5_prefix=`$BREW --prefix qt5 2>/dev/null`
         if test x$openssl_prefix != x; then
           PKG_CONFIG_PATH="$openssl_prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
           export PKG_CONFIG_PATH
         fi
         if test x$qt5_prefix != x; then
           PKG_CONFIG_PATH="$qt5_prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
           export PKG_CONFIG_PATH
         fi
       fi
     else
       case $build_os in
         *darwin*)
           BUILD_OS=darwin
           ;;
         *)
           AC_PATH_TOOL([INSTALLNAMETOOL], [install_name_tool], install_name_tool)
           AC_PATH_TOOL([OTOOL], [otool], otool)
           AC_PATH_PROGS([GENISOIMAGE], [genisoimage mkisofs],genisoimage)

           dnl libtool will try to strip the static lib, which is a problem for
           dnl cross-builds because strip attempts to call a hard-coded ld,
           dnl which may not exist in the path. Stripping the .a is not
           dnl necessary, so just disable it.
           old_striplib=
           ;;
       esac
     fi

     AX_CHECK_LINK_FLAG([[-Wl,-headerpad_max_install_names]], [LDFLAGS="$LDFLAGS -Wl,-headerpad_max_install_names"])
     CPPFLAGS="$CPPFLAGS -DMAC_OSX"
     ;;
   *linux*)
     TARGET_OS=linux
     ;;
   *)
     ;;
esac

if test x$use_pkgconfig = xyes; then

  if test x"$PKG_CONFIG" = "x"; then
    AC_MSG_ERROR(pkg-config not found.)
  fi

  : #NOP
  m4_ifdef(
    [PKG_CHECK_MODULES],
    [
      PKG_CHECK_MODULES([SSL], [libssl],, [AC_MSG_ERROR(openssl  not found.)])
      PKG_CHECK_MODULES([CRYPTO], [libcrypto],,[AC_MSG_ERROR(libcrypto  not found.)])
    ]
  )
else
  AC_CHECK_HEADER([openssl/crypto.h],,AC_MSG_ERROR(libcrypto headers missing))
  AC_CHECK_LIB([crypto],      [main],CRYPTO_LIBS=-lcrypto, AC_MSG_ERROR(libcrypto missing))

  AC_CHECK_HEADER([openssl/ssl.h],, AC_MSG_ERROR(libssl headers missing),)
  AC_CHECK_LIB([ssl],         [main],SSL_LIBS=-lssl, AC_MSG_ERROR(libssl missing))
fi

BITCOIN_QT_INIT
dnl sets $bitcoin_enable_qt, $bitcoin_enable_qt_test, $bitcoin_enable_qt_dbus
BITCOIN_QT_CONFIGURE([$use_pkgconfig], [qt5])

if test x$enable_daemon = xyes; then
	if test x$bitcoin_enable_qt = xyes; then
	AC_DEFINE_UNQUOTED([ENABLE_QT],[1],[Define to 1 to enable Qt Gui])
	fi
fi

AM_CONDITIONAL([ENABLE_QT],[test x$bitcoin_enable_qt = xyes])
AM_CONDITIONAL([ENABLE_DAEMON],[test x$enable_daemon = xyes])

AC_PROG_CXX
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
