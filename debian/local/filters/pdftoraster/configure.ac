dnl  configure.ac -- an -*- autoconf -*- template for configure
dnl  Copyright (C) 2008  BBR Inc.
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

AC_INIT([PDF to raster filter for CUPS],
  [0.0.1],
  ,
  [pdftoraster])

AC_CONFIG_SRCDIR([src/pdftoraster.cc])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([pdftoraster],0.0.1)

dnl poppler source dir

AC_ARG_WITH([poppler-source],[  --with-poppler-source=PATH	poppler source directory path],
  [POPPLER_SRCDIR=$withval],
  [POPPLER_SRCDIR=`eval echo $includedir`])
AC_SUBST(POPPLER_SRCDIR)

dnl cups build dir

AC_ARG_WITH([cups-build],[  --with-cups-build=PATH	cups >= 1.2 build directory path],
  [CUPS_BUILD=$withval],)
AC_SUBST(CUPS_BUILD)

dnl checks for programs

AC_PROG_LIBTOOL
AC_PROG_CC
AC_C_CONST
AC_ISC_POSIX
AC_PROG_CC_STDC
AC_PROG_CXX
AC_PROG_INSTALL
AC_HEADER_DIRENT
AM_PROG_CC_C_O

dnl checks for libraries

dnl check zlib

AC_CHECK_LIB(z,deflate)

dnl check cups

AC_CHECK_LIB(cups,ppdOpenFile,,
  [ echo "*** cups library not found. ***";exit ]
)

if test x$CUPS_BUILD = x;then
  AC_CHECK_LIB(cupsimage,cupsRasterInterpretPPD,
    [
      LIBS="$LIBS -lcupsimage"
      HAVE_LIBCUPSIMAGE=yes
      AC_DEFINE(HAVE_LIBCUPSIMAGE,[],[cupsimage lib])
    ],
    [ echo "*** cupsimage library >= 1.2 not found. please specify --with-cups-build ***";exit ]
  )
else
    LIBS="$LIBS $CUPS_BUILD/filter/libcupsimage.a"
    CPPFLAGS="$CPPFLAGS -I${CUPS_BUILD}"
    HAVE_LIBCUPSIMAGE=yes
    AC_DEFINE(HAVE_LIBCUPSIMAGE,[],[cupsimage lib])
fi

dnl Switch over to C++.
AC_LANG(C++)

dnl check poppler
AC_CHECK_LIB(poppler,main,,
  [ echo "*** poppler library not found. ***";exit ]
)

dnl checks for header files

AC_CHECK_HEADERS([zlib.h])

dnl checks for functions

dnl checks for tools

AC_CHECK_PROG(CUPS_SERVERBIN,cups-config,
  [`cups-config --serverbin`],[/usr/lib/cups])
AC_CHECK_PROG(CUPS_SERVERROOT,cups-config,
  [`cups-config --serverroot`],[/etc/cups])

AC_SUBST(CUPS_SERVERBIN)
AC_SUBST(CUPS_SERVERROOT)

AC_ARG_ENABLE(compile-warnings,
	    AC_HELP_STRING([--enable-compile-warnings]
			   [Turn on compiler warnings.]),,
	    [enable_compile_warnings="yes"])

if test x"$enable_compile_warnings" == xyes ;then
   CXXFLAGS="-Wall $CXXFLAGS"
fi

dnl check if GlobalParams::GlobalParams has a argument
if grep "GlobalParams(char \*cfgFileName)" $POPPLER_SRCDIR/poppler/GlobalParams.h >/dev/null ;then
    AC_DEFINE([GLOBALPARAMS_HAS_A_ARG],,[GlobalParams::GlobalParams has a argument.])
fi

AC_CONFIG_FILES([Makefile src/Makefile conf/Makefile])
AC_OUTPUT
