
bin_PROGRAMS = mpeg2video_parse

mpeg2video_parse_SOURCES = \
	main.cpp

INCLUDES = -I$(srcdir) -I$(top_srcdir)/common/video/mpeg4 -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/mp4av 

AM_CFLAGS= -D_REENTRANT @BILLS_CWARNINGS@ 

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ 

mpeg2video_parse_LDADD = \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la 

EXTRA_DIST=
