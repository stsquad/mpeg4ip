###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libxvidimagex86.la
libxvidimagex86_la_LDFLAGS = -static

libxvidimagex86_la_SOURCES = dummy.c

ASMFILES = \
	interpolate8x8_mmx.asm \
	rgb_to_yv12_mmx.asm \
	yuv_to_yv12_mmx.asm \
	yuyv_to_yv12_mmx.asm \
	yv12_to_rgb24_mmx.asm \
	yv12_to_rgb32_mmx.asm \
	yv12_to_yuyv_mmx.asm

if USE_MMX
EXTRA_libxvidimagex86_la_SOURCES = $(ASMFILES)

libxvidimagex86_la_LIBADD = \
	interpolate8x8_mmx.lo \
	rgb_to_yv12_mmx.lo \
	yuv_to_yv12_mmx.lo \
	yuyv_to_yv12_mmx.lo \
	yv12_to_rgb24_mmx.lo \
	yv12_to_rgb32_mmx.lo \
	yv12_to_yuyv_mmx.lo
endif

EXTRA_DIST = $(ASMFILES)
