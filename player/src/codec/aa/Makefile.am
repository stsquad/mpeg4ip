libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = aac_plugin.la
aac_plugin_la_LDFLAGS = -module
aac_plugin_la_SOURCES = \
	aac.cpp \
	aa_file.cpp

aac_plugin_la_LIBADD = \
	$(top_builddir)/player/lib/audio/faad/libfaad.la \
	-lm

noinst_LTLIBRARIES = libaa.la
libaa_la_LDFLAGS = -static
libaa_la_SOURCES = \
	isma_rtp_bytestream.cpp

INCLUDES = -I../.. -I../../../lib -I../../../lib/audio -I$(top_srcdir)/include -I$(top_srcdir)/lib/SDL/include -I../../../../lib

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

