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


INCLUDES =  -I$(top_srcdir)/include -I$(top_srcdir)/lib/SDL/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/sdp -I$(top_srcdir)/player/src -I$(top_srcdir)/player/lib/audio -I$(top_srcdir)/player/lib -I$(top_srcdir)/lib/mp4v2

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = aac_plugin_60.dsp
