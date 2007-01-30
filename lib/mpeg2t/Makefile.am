noinst_LTLIBRARIES = libmpeg2_transport.la
libmpeg2_transport_la_SOURCES = \
	mpeg2_transport.c \
	mpeg2_transport.h \
	mpeg2t_ac3.c \
	mpeg2t_mp3.c \
	mpeg2t_private.h \
	mpeg2t_defines.h \
	mpeg2t_video.c \
	mpeg2t_util.c 

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/mp4av -I$(top_srcdir)/lib/mp4v2 
AM_CFLAGS = -D_REENTRANT @BILLS_CWARNINGS@
AM_CXXFLAGS = -D_REENTRANT @BILLS_CPPWARNINGS@

bin_PROGRAMS = mpeg2t_dump

check_PROGRAMS = mpeg2t_test mpeg2t_extract

mpeg2t_dump_SOURCES = mpeg2t_dump.cpp
mpeg2t_dump_LDADD = libmpeg2_transport.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	@SDL_LIBS@ 

mpeg2t_test_SOURCES = test.cpp
mpeg2t_test_LDADD = libmpeg2_transport.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	@SDL_LIBS@ 

mpeg2t_extract_SOURCES = mpeg2t_extract.cpp
mpeg2t_extract_LDADD = libmpeg2_transport.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	@SDL_LIBS@ 

EXTRA_DIST= 
