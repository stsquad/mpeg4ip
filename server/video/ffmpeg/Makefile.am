###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

lib_LTLIBRARIES = libffmpeg.la
libffmpeg_la_LDFLAGS = -static

INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = -DMPEG4IP -O2

if USE_MMX
EXTRA_libffmpeg_la_SOURCES = \
	text_fdct_mmx.asm 
libffmpeg_la_LIBADD = \
	text_fdct_mmx.lo
else
EXTRA_libffmpeg_la_SOURCES = 
libffmpeg_la_LIBADD = 
endif

libffmpeg_la_SOURCES = \
	avcodec.h \
	common.c \
	common.h \
	dsputil.c \
	dsputil.h \
	h263data.h \
	h263enc.c \
	jfdctfst.c \
	jrevdct.c \
	motion_est.c \
	mjpegenc.c \
	mpeg4data.h \
	mpegencodevlc.h \
	mpegvideo.c \
	mpegvideo.h 

