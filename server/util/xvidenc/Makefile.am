bin_PROGRAMS = xvidenc
xvidenc_SOURCES = xvidenc.c
if USE_MPEG4IP_XVID
  XVID_LIB=$(top_builddir)/lib/xvid/libmpeg4ip_xvid.la
  XVID_INC=-I$(top_srcdir)/lib/xvid
else
  XVID_LIB=-lxvidcore
  XVID_INC=
endif

INCLUDES = \
	-I$(top_srcdir)/include \
	$(XVID_INC)

LDADD = \
	$(XVID_LIB) \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	 -lm
EXTRA_DIST = xvidenc.dsp
