bin_PROGRAMS = avi2raw
avi2raw_SOURCES = avi2raw.c
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib/avi -I$(top_srcdir)/lib/mp4av
LDADD = $(top_builddir)/lib/avi/libavi.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	 -lm 
EXTRA_DIST = avi2raw60.dsp 
