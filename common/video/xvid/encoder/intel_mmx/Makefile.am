###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libxvidencodermmx.la
libxvidencodermmx_la_LDFLAGS = -static

libxvidencodermmx_la_SOURCES = \
	cbp_mmx.asm \
	sad_mmx.asm
