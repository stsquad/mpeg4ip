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
	interpolate8x8_mmx.asm \
	quantize_mmx.asm \
	quantize4_mmx.asm \
	transfer_mmx.asm

am_libxvidcommonmmx_la_OBJECTS = \
	compensate_mmx.lo \
	cpuid.lo \
	fdct_mmx.lo \
	idct_mmx.lo \
	interpolate8x8_mmx.lo \
	quantize_mmx.lo \
	quantize4_mmx.lo \
	transfer_mmx.lo
