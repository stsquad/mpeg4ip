bin_PROGRAMS = xvidenc
xvidenc_SOURCES = xvidenc.c
if USE_MPEG4IP_XVID
  XVID_LIB=$(top_builddir)/lib/xvid/libmpeg4ip_xvid.la
else
if HAVE_XVID_1_0
  XVID_LIB=$(top_builddir)/lib/xvid/libmpeg4ip_xvid.la
else
  XVID_LIB=-lxvidcore
endif
endif

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/mp4av \
	-I$(top_srcdir)/lib/mp4v2 

LDADD = \
	$(XVID_LIB) \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	 -lm
EXTRA_DIST = xvidenc.dsp
