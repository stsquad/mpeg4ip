
bin_PROGRAMS = mpeg4vol

mpeg4vol_SOURCES = \
	main.cpp audio.cpp

INCLUDES = -I$(srcdir) -I$(top_srcdir)/common/video/mpeg4 -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/mp4av 

AM_CFLAGS= -D_REENTRANT @BILLS_CWARNINGS@ 

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ 

mpeg4vol_LDADD = \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la 

EXTRA_DIST=mpeg4vol.dsp
