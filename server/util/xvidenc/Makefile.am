bin_PROGRAMS = xvidenc
xvidenc_SOURCES = xvidenc.c

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/xvid

LDADD = \
	$(top_builddir)/lib/xvid/libmpeg4ip_xvid.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	 -lm
EXTRA_DIST = xvidenc.dsp
