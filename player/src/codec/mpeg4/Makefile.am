libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = mpeg4_iso_plugin.la
mpeg4_iso_plugin_la_LDFLAGS = -module
mpeg4_iso_plugin_la_SOURCES = \
	mpeg4.cpp \
	mpeg4.h \
	mpeg4_file.cpp \
	mpeg4_file.h

mpeg4_dir=common/video/iso-mpeg4

#
# note - remove -lstdc++ from here, 2/27/04 - looks like it was needed
# for Solaris
#
mpeg4_iso_plugin_la_LIBADD = \
	-lm @SUN_LIBS@ \
	$(top_builddir)/lib/fposrec/libmpeg4ip_fposrec.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/$(mpeg4_dir)/src/libmpeg4ip_mpeg4_iso.la \
	$(top_builddir)/lib/mp4av/libmp4av.la 

INCLUDES = \
	-I$(top_srcdir)/player/src \
	-I$(top_srcdir)/player/lib \
	-I$(top_srcdir)/$(mpeg4_dir) \
	-I$(top_builddir)/$(mpeg4_dir) \
	-I$(top_srcdir)/$(mpeg4_dir)/include \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/sdp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@ -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_OBSS_


AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_OBSS_

EXTRA_DIST = mpeg4_iso_plugin60.dsp mpeg4_iso_plugin.vcproj
