###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libxviddecodermmx.la
libxviddecodermmx_la_LDFLAGS = -static

libxviddecodermmx_la_SOURCES = \
	compensate_halfpel_mmx.asm
