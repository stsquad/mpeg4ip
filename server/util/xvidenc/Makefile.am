bin_PROGRAMS = xvidenc
xvidenc_SOURCES = xvidenc.c

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/xvid

LDADD = \
	$(top_builddir)/lib/xvid/libxvid.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	 -lm

