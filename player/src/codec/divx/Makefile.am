libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = divx_plugin.la
divx_plugin_la_LDFLAGS = -module
divx_plugin_la_SOURCES = \
	divx.cpp \
	divx_file.cpp

divx_plugin_la_LIBADD = \
	-lm \
	$(top_builddir)/player/lib/video/divx/libourdivx.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/fposrec/libfposrec.la

INCLUDES = -I.. -I../.. -I../../../lib -I$(top_srcdir)/lib/SDL/include -I../../../lib/video/divx -I../../../../common/video/mpeg4 -I../../../../include -I../../../../lib

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

