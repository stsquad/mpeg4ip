libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = aac_plugin.la
aac_plugin_la_LDFLAGS = -module
aac_plugin_la_SOURCES = \
	aac.cpp \
	aac.h \
	aa_file.cpp \
	aa_file.h

aac_plugin_la_LIBADD = \
	$(top_builddir)/player/lib/audio/faad/libfaad.la \
	-lm


INCLUDES = -I../.. -I../../../lib -I../../../lib/audio -I$(top_srcdir)/include -I$(top_srcdir)/lib/SDL/include -I../../../../lib

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = aac_plugin_60.dsp
