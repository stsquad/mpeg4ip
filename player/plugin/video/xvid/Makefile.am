libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = xvid_plugin.la
xvid_plugin_la_LDFLAGS = -module
xvid_plugin_la_SOURCES = \
	ourxvid.cpp \
	ourxvid.h \
	xvid_file.cpp

if USE_MPEG4IP_XVID
  XVID_LIB=$(top_builddir)/lib/xvid/libmpeg4ip_xvid.la
  XVID_INC=-I$(top_srcdir)/lib/xvid
else
  XVID_LIB=-lxvidcore
  XVID_INC=
endif

xvid_plugin_la_LIBADD = \
	-lm \
	$(XVID_LIB) \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/fposrec/libfposrec.la \
	$(top_builddir)/lib/sdp/libsdp.la \
	$(top_builddir)/lib/mp4av/libmp4av.la

INCLUDES = -I.. -I../.. -I../../../lib -I$(top_srcdir)/lib/SDL/include $(XVID_INC) -I../../../../include -I../../../../lib -I$(top_srcdir)/player/src -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/lib/sdp -I$(top_srcdir)/lib/mp4av


AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = xvid_plugin.dsp
