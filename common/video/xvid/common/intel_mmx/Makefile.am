###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libxvidcommonmmx.la
libxvidcommonmmx_la_LDFLAGS = -static

libxvidcommonmmx_la_SOURCES = \
	compensate_mmx.asm \
	cpuid.asm \
	fdct_mmx.asm \
	idct_mmx.asm \
	quantize_mmx.asm \
	transfer_mmx.asm

