libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = xvid_plugin.la
xvid_plugin_la_LDFLAGS = -module
xvid_plugin_la_SOURCES = \
	ourxvid.cpp \
	xvid_file.cpp

xvid_plugin_la_LIBADD = \
	-lm \
	$(top_builddir)/lib/xvid/libxvid.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/fposrec/libfposrec.la

INCLUDES = -I.. -I../.. -I../../../lib -I$(top_srcdir)/lib/SDL/include -I$(top_srcdir)/lib/xvid -I../../../../include -I../../../../lib -I$(top_srcdir)/player/src

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

