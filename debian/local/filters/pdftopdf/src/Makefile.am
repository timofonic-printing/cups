##  Makefile.am -- an -*- automake -*- template for Makefile.in
##  Copyright (C) 2006-2007  BBR Inc.
##
##  Permission is hereby granted, free of charge, to any person
##  obtaining a copy of this software and associated documentation files
##  (the "Software"), to deal in the Software without restriction,
##  including without limitation the rights to use, copy, modify, merge,
##  publish, distribute, sublicense, and/or sell copies of the Software,
##  and to permit persons to whom the Software is furnished to do so,
##  subject to the following conditions:
##
##  The above copyright notice and this permission notice shall be
##  included in all copies or substantial portions of the Software.
##
##  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
##  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
##  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
##  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
##  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
##  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
##  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
##  SOFTWARE.
##
##  Process this file with automake to make a Makefile.in file.

bin_PROGRAMS=pdf2pdf
pdf2pdf_CPPFLAGS = -I@POPPLER_SRCDIR@/poppler \
	-I@POPPLER_SRCDIR@ -DCONFFILE_DIR=\"@CUPS_SERVERROOT@\" \
	-DPDFTOPDF

pdf2pdf_SOURCES = \
	P2PCatalog.cc P2PCatalog.h\
	P2PDoc.cc P2PDoc.h P2PGfx.cc P2PGfx.h\
	P2PMatrix.h P2PObject.cc P2PObject.h P2POutput.cc P2POutput.h\
	P2POutputStream.cc P2POutputStream.h P2PPage.cc P2PPage.h\
	P2PPageTree.cc P2PPageTree.h P2PResources.cc P2PResources.h\
	P2PXRef.cc P2PXRef.h pdf2pdf.cc P2PFont.cc P2PFont.h\
	PDFFTrueTypeFont.cc PDFFTrueTypeFont.h P2PPattern.cc P2PPattern.h\
	P2PCharCodeToUnicode.cc P2PCharCodeToUnicode.h\
	P2PCMap.cc P2PCMap.h parseargs.h parseargs.c\
	UGooString.cc UGooString.h

filterdir = @CUPS_SERVERBIN@/filter
filter_PROGRAMS=pdftopdf

pdftopdf_CPPFLAGS = -I@POPPLER_SRCDIR@/poppler\
	-I@POPPLER_SRCDIR@ -DCONFFILE_DIR=\"@CUPS_SERVERROOT@\" \
	-DPDFTOPDF

pdftopdf_SOURCES = \
	P2PCatalog.cc P2PCatalog.h\
	P2PDoc.cc P2PDoc.h P2PGfx.cc P2PGfx.h\
	P2PMatrix.h P2PObject.cc P2PObject.h P2POutput.cc P2POutput.h\
	P2POutputStream.cc P2POutputStream.h P2PPage.cc P2PPage.h\
	P2PPageTree.cc P2PPageTree.h P2PResources.cc P2PResources.h\
	P2PXRef.cc P2PXRef.h pdftopdf.cc P2PFont.cc P2PFont.h\
	PDFFTrueTypeFont.cc PDFFTrueTypeFont.h P2PPattern.cc P2PPattern.h\
	P2PCharCodeToUnicode.cc P2PCharCodeToUnicode.h\
	P2PCMap.cc P2PCMap.h UGooString.cc UGooString.h
