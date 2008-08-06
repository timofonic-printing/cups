dnl  configure.ac -- an -*- autoconf -*- template for configure
dnl  Copyright (C) 2006  BBR Inc.
dnl
dnl  Permission is hereby granted, free of charge, to any person
dnl  obtaining a copy of this software and associated documentation files
dnl  (the "Software"), to deal in the Software without restriction,
dnl  including without limitation the rights to use, copy, modify, merge,
dnl  publish, distribute, sublicense, and/or sell copies of the Software,
dnl  and to permit persons to whom the Software is furnished to do so,
dnl  subject to the following conditions:
dnl
dnl  The above copyright notice and this permission notice shall be
dnl  included in all copies or substantial portions of the Software.
dnl
dnl  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
dnl  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
dnl  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
dnl  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
dnl  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
dnl  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
dnl  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
dnl  SOFTWARE.
dnl
dnl  Process this file with autoconf to make a configure file.

AC_INIT([Image to PDF filter for CUPS],
  [0.0.1],
  ,
  [imagetopdf])

AC_CONFIG_SRCDIR([imagetopdf.c])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([imagetopdf],0.0.2)

dnl checks for programs

AC_PROG_CC
AC_C_CONST
AC_PROG_INSTALL
AM_PROG_CC_C_O

dnl checks for libraries

AC_CHECK_LIB(cups,ppdEmitJCLEnd,,
  AC_CHECK_LIB(cups,ppdOpenFile,
    [
      LIBS="$LIBS -lcups"
      HAVE_LIBCUPS=yes
      CUPS_1_1=yes
      AC_DEFINE(HAVE_LIBCUPS)
      AC_DEFINE(CUPS_1_1,,[ CUPS Version is 1.1 ])
    ],
    [ echo "*** cups library not found. ***";exit ]
  )
)

AC_CHECK_LIB(cupsimage,cupsImageOpen,,
  AC_CHECK_LIB(cupsimage,ImageOpen,
    [
      LIBS="$LIBS -lcupsimage"
      HAVE_LIBCUPSIMAGE=yes
      CUPS_1_1=yes
      AC_DEFINE(HAVE_LIBCUPSIMAGE)
      AC_DEFINE(CUPS_1_1,,[ CUPS Version is 1.1 ])
    ],
    [ echo "*** cupsimage library not found. ";exit ]
  )
)

dnl checks for header files

AC_CHECK_HEADERS(string.h strings.h)

dnl checks for functions

AC_CHECK_FUNCS(strdup strcasecmp strncasecmp strlcat strlcpy snprintf vsnprintf)

dnl checks for tools

AC_CHECK_PROG(CUPS_SERVERBIN,cups-config,
  [`cups-config --serverbin`],[/usr/lib/cups])
AC_CHECK_PROG(CUPS_SERVERROOT,cups-config,
  [`cups-config --serverroot`],[/etc/cups])
AC_PATH_PROG(CONVERT_CMD,convert,
  ,/usr/X11R6/bin)
AC_SUBST(CUPS_SERVERBIN)
AC_SUBST(CUPS_SERVERROOT)

AH_TEMPLATE(CONVERT_CMD,[ convert command path ])
if test x$CONVERT_CMD != x ;then
  AC_DEFINE_UNQUOTED(CONVERT_CMD,"$CONVERT_CMD")
fi
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
