noinst_LTLIBRARIES = libmpeg2_transport.la
libmpeg2_transport_la_LDFLAGS = -static
libmpeg2_transport_la_SOURCES = \
	mpeg2_transport.c \
	mpeg2_transport.h \
	mpeg2t_mp3.c \
	mpeg2t_private.h \
	mpeg2t_video.c \
	mpeg2t_util.c 

INCLUDES = -I$(top_builddir)/include -I$(top_builddir)/lib -I$(top_builddir)/lib/mp4av -I$(top_builddir)/lib/mp4v2 -I$(top_builddir)/lib/SDL/include
AM_CFLAGS = -DIPTV_COMPATIBLE -D_REENTRANT @BILLS_CWARNINGS@
AM_CXXFLAGS = -D_REENTRANT @BILLS_CPPWARNINGS@

check_PROGRAMS = mpeg2t_test 

mpeg2t_test_SOURCES = test.cpp
mpeg2t_test_LDADD = libmpeg2_transport.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	@SDL_LIBS@
