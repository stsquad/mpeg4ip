###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libxvidbitstreamx86.la

libxvidbitstreamx86_la_SOURCES = dummy.c 

if USE_MMX
EXTRA_libxvidbitstreamx86_la_SOURCES = cbp_mmx.asm
libxvidbitstreamx86_la_LIBADD = cbp_mmx.lo
endif

libxvidbitstreamx86_la_LDFLAGS = -static

EXTRA_DIST=cbp_mmx.asm
