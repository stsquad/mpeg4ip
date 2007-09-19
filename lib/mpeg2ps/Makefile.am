noinst_LTLIBRARIES = libmpeg2_program.la
libmpeg2_program_la_SOURCES = \
	mpeg2ps.c \
	mpeg2ps_private.h \
	mpeg2_ps.h \
	mpeg2ps_util.c


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/mp4av -I$(top_srcdir)/lib/mp4v2 
AM_CFLAGS = -D_REENTRANT @BILLS_CWARNINGS@
AM_CXXFLAGS = -D_REENTRANT @BILLS_CPPWARNINGS@

bin_PROGRAMS = mpeg_ps_info mpeg_ps_extract
prog_libadd = \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	libmpeg2_program.la 

mpeg_ps_info_SOURCES = ps_info.cpp
mpeg_ps_info_LDADD = $(prog_libadd)
mpeg_ps_extract_SOURCES = ps_extract.cpp
mpeg_ps_extract_LDADD = $(prog_libadd)
check_PROGRAMS = mpeg2ps_test 

mpeg2ps_test_SOURCES = test.cpp
mpeg2ps_test_LDADD = libmpeg2_program.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	libmpeg2_program.la 

EXTRA_DIST=libmpeg2ps.dsp libmpeg2ps.vcproj mpeg_ps_info.vcproj

