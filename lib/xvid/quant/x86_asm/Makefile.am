###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libxvidquantx86.la
libxvidquantx86_la_LDFLAGS = -static

libxvidquantx86_la_SOURCES = \
	quantize_mmx.asm \
	quantize4_mmx.asm

am_libxvidquantx86_la_OBJECTS = \
	quantize_mmx.lo \
	quantize4_mmx.lo

