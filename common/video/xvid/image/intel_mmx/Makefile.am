###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libxvidimagemmx.la
libxvidimagemmx_la_LDFLAGS = -static

libxvidimagemmx_la_SOURCES = \
	halfpel_mmx.asm \
	in_rgb_mmx.asm \
	in_yuv_mmx.asm \
	in_yuyv_mmx.asm \
	out_rgb24_mmx.asm \
	out_rgb32_mmx.asm \
	out_yuyv_mmx.asm

