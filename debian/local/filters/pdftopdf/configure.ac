dnl  configure.ac -- an -*- autoconf -*- template for configure
dnl  Copyright (C) 2006-2007  BBR Inc.
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

AC_INIT([PDF to PDF filter for CUPS],
  [0.0.1],
  ,
  [pdftopdf])

AC_CONFIG_SRCDIR([src/pdftopdf.cc])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([pdftopdf],0.0.1)

dnl poppler source dir

AC_ARG_WITH([poppler-source],[  --with-poppler-source=PATH	poppler source directory path],
  [POPPLER_SRCDIR=$withval],
  [POPPLER_SRCDIR=`eval echo $includedir`])
AC_SUBST(POPPLER_SRCDIR)

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

dnl check if Praser:Praser has two arguments
if grep "Parser(XRef \*xrefA, Lexer \*lexerA)" $POPPLER_SRCDIR/poppler/Parser.h >/dev/null ;then
    AC_DEFINE([PARSER_HAS_2_ARGS],,[Parser::Parser has two arguments.])
fi

dnl check font type enumeration
if grep "fontType1COT" $POPPLER_SRCDIR/poppler/GfxFont.h >/dev/null ;then
    AC_DEFINE([FONTTYPE_ENUM2],,[New font type enumeration])
fi

dnl check Stream::getUndecodedStream
if grep "getUndecodedStream" $POPPLER_SRCDIR/poppler/Stream.h >/dev/null ;then
    AC_DEFINE([HAVE_GETUNDECODEDSTREAM],,[Have Stream::getUndecodedStrem])
fi

dnl check UGooString.h
CPPFLAGS="$CPPFLAGS -I$POPPLER_SRCDIR/poppler"
AC_CHECK_HEADER(UGooString.h,
    AC_DEFINE([HAVE_UGOOSTRING_H],,[Have UGooString.g])
,)

AC_CONFIG_FILES([Makefile src/Makefile conf/Makefile man/Makefile])
AC_OUTPUT
