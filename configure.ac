#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ffnes],[2.0.0],[chenk@apical.com.cn])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/libffnes.pc])

AM_INIT_AUTOMAKE([subdir-objects])

# Checks for programs.
AC_PROG_CC
LT_INIT

# Checks for pkg-config
PKG_PROG_PKG_CONFIG

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])

CFLAGS="-Ofast -g -Wall -Wno-strict-aliasing -funwind-tables -ffunction-sections -fdata-sections"

# Checks for libavdev
AC_ARG_WITH([libavdev], [AS_HELP_STRING([--with-libavdev], [--with-libavdev=PATH, use libavdev, specify path of libavdev])], [], [with_libavdev="no"])
AS_IF([test "x$with_libavdev" != "xno"],
  [ CFLAGS="$CFLAGS -DWITH_LIBAVDEV -I$with_libavdev/include"
    CPPFLAGS="$CPPFLAGS -DWITH_LIBAVDEV -I$with_libavdev/include"
    LDFLAGS="$LDFLAGS -L$with_libavdev/lib -lavdev"
  ])

# Checks for Optional Features
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [enable debug mode [default=no]])], [], [debug=no])
AM_CONDITIONAL([ENABLE_DEBUG], [test x"$enable_debug" = x"yes"])

AC_OUTPUT
