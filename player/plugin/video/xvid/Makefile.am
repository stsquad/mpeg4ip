libdir = @PLAYER_PLUGIN_DIR@

if HAVE_XVID_1_0
plugin=xvid10_plugin.la
else
if NO_XVID
plugin=
else
plugin=xvid_plugin.la
endif
endif

lib_LTLIBRARIES = $(plugin) 
if HAVE_XVID_1_0
xvid10_plugin_la_LDFLAGS = -module
xvid10_plugin_la_SOURCES = \
	xvid1_0.cpp 

xvid10_plugin_la_LIBADD = \
	-lm \
	-lxvidcore \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/player/lib/mp4util/libmp4util.la \
	$(top_builddir)/lib/fposrec/libmpeg4ip_fposrec.la \
	$(top_builddir)/lib/sdp/libsdp.la \
	$(top_builddir)/lib/mp4av/libmp4av.la
endif

if NO_XVID

else
  XVID_LIB=-lxvidcore
xvid_plugin_la_LDFLAGS = -module
xvid_plugin_la_SOURCES = \
	ourxvid.cpp \
	ourxvid.h \
	xvid_file.cpp

xvid_plugin_la_LIBADD = \
	-lm \
	-lxvidcore \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/player/lib/mp4util/libmp4util.la \
	$(top_builddir)/lib/fposrec/libmpeg4ip_fposrec.la \
	$(top_builddir)/lib/sdp/libsdp.la \
	$(top_builddir)/lib/mp4av/libmp4av.la

endif
INCLUDES = -I$(top_srcdir)/player/lib  -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/player/src -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/lib/sdp -I$(top_srcdir)/lib/mp4av


AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = 
