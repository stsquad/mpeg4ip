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

ASMFILES = \
	quantize_mmx.asm \
	quantize4_mmx.asm

libxvidquantx86_la_SOURCES = dummy.c

if USE_MMX
EXTRA_libxvidquantx86_la_SOURCES = $(ASMFILES)

libxvidquantx86_la_LIBADD = \
	quantize_mmx.lo \
	quantize4_mmx.lo
endif
EXTRA_DIST = $(ASMFILES)

