libdir = @PLAYER_PLUGIN_DIR@

if HAVE_XVID_1_0
extraplugin=xvid10_plugin.la
endif

lib_LTLIBRARIES = xvid_plugin.la $(extraplugin)
if HAVE_XVID_1_0
xvid10_plugin_la_LDFLAGS = -module
xvid10_plugin_la_SOURCES = \
	xvid1_0.cpp 

xvid10_plugin_la_LIBADD = \
	-lm \
	-lxvidcore \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/player/lib/mp4util/libmp4util.la \
	$(top_builddir)/lib/fposrec/libfposrec.la \
	$(top_builddir)/lib/sdp/libsdp.la \
	$(top_builddir)/lib/mp4av/libmp4av.la
  XVID_LIB=$(top_builddir)/lib/xvid/libmpeg4ip_xvid.la
else

if USE_MPEG4IP_XVID
  XVID_LIB=$(top_builddir)/lib/xvid/libmpeg4ip_xvid.la
else
  XVID_LIB=-lxvidcore
endif
endif
xvid_plugin_la_LDFLAGS = -module
xvid_plugin_la_SOURCES = \
	ourxvid.cpp \
	ourxvid.h \
	xvid_file.cpp


xvid_plugin_la_LIBADD = \
	-lm \
	$(XVID_LIB) \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/player/lib/mp4util/libmp4util.la \
	$(top_builddir)/lib/fposrec/libfposrec.la \
	$(top_builddir)/lib/sdp/libsdp.la \
	$(top_builddir)/lib/mp4av/libmp4av.la

INCLUDES = -I.. -I../.. -I../../../lib  -I../../../../include -I../../../../lib -I$(top_srcdir)/player/src -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/lib/sdp -I$(top_srcdir)/lib/mp4av


AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = xvid_plugin.dsp xvid_plugin.xcode/project.pbxproj
