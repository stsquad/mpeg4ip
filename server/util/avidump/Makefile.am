bin_PROGRAMS = avidump
avidump_SOURCES = avidump.c
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib/avi
LDADD = $(top_builddir)/lib/avi/libavi.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	 -lm
EXTRA_DIST = avidump60.dsp avidump.vcproj
