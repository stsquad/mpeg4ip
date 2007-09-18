
bin_PROGRAMS = h264_parse
check_PROGRAMS = strip_bytes

h264_parse_SOURCES = \
	main.cpp

strip_bytes_SOURCES = strip.cpp

INCLUDES = -I$(srcdir) -I$(top_srcdir)/common/video/mpeg4 -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/mp4av 

AM_CFLAGS= -D_REENTRANT @BILLS_CWARNINGS@ 

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ 

h264_parse_LDADD = \
	-lm \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la 

EXTRA_DIST= h264_parse.dsp h264_parse.vcproj
