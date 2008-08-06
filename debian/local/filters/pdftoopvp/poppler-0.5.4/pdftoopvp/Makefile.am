INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/poppler			\
	-I$(top_srcdir)/oprs			\
	-I$(top_srcdir)/goo			\
	-I$(top_srcdir)/splash			\
	-I$(top_srcdir)/fofi			\
	-I$(top_srcdir)/opvp/vector-printer	\
	$(FREETYPE_CFLAGS)			\
	$(CUPS_CFLAGS)				\
	$(FONTCONFIG_CFLAGS)			\
	$(PDFTOOPVP_CFLAGS)

LDADD =						\
	$(top_builddir)/poppler/.libs/libpoppler.a	\
	$(top_builddir)/oprs/liboprs.la		\
	$(CUPS_LIBS)				\
	$(FONTCONFIG_LIBS)			\
	$(CAIRO_LIBS)                           \
	$(LIBJPEG_LIBS)                         \
	$(ZLIB_LIBS)                            \
	$(FREETYPE_LIBS)			\
	$(PDFTOOPVP_LIBS)


filterdir = @CUPS_SERVERBIN@/filter

filter_PROGRAMS =				\
	pdftoopvp

pdftoopvp_SOURCES =				\
	pdftoopvp.cc				\
	OPVPOutputDev.cc			\
	OPVPOutputDev.h


pdftoopvp_CXXFLAGS=-DCUPS_SERVERROOT=\"@CUPS_SERVERROOT@\"

pdftoopvp_binary = pdftoopvp

fontconfdir = $(sysconfdir)/fonts/conf.d

fontconf_DATA = 99pdftoopvp.conf
