libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = xvid_plugin.la
xvid_plugin_la_LDFLAGS = -module
xvid_plugin_la_SOURCES = \
	ourxvid.cpp \
	ourxvid.h \
	xvid_file.cpp

xvid_plugin_la_LIBADD = \
	-lm \
	$(top_builddir)/lib/xvid/libxvid.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/fposrec/libfposrec.la \
	$(top_builddir)/lib/mp4av/libmp4av.la

INCLUDES = -I.. -I../.. -I../../../lib -I$(top_srcdir)/lib/SDL/include -I$(top_srcdir)/lib/xvid -I../../../../include -I../../../../lib -I$(top_srcdir)/player/src -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/lib/sdp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = xvid_plugin.dsp
