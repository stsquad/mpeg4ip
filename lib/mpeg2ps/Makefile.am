noinst_LTLIBRARIES = libmpeg2_program.la
libmpeg2_program_la_SOURCES = \
	mpeg2ps.c \
	mpeg2_ps.h 


INCLUDES = -I$(top_builddir)/include -I$(top_builddir)/lib -I$(top_builddir)/lib/mp4av -I$(top_builddir)/lib/mp4v2 
AM_CFLAGS = -D_REENTRANT @BILLS_CWARNINGS@
AM_CXXFLAGS = -D_REENTRANT @BILLS_CPPWARNINGS@

check_PROGRAMS = mpeg2ps_test 

mpeg2ps_test_SOURCES = test.cpp
mpeg2ps_test_LDADD = libmpeg2_program.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	libmpeg2_program.la \
	@SDL_LIBS@ 

